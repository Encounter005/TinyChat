# 文件服务器断点续传功能设���文档

**日期**: 2026-02-06
**版本**: 1.0
**作者**: Claude Code

---

## 1. 概述

本文档描述了为 TinyChat 文件服务器添加断点续传功能的完整设计方案。断点续传支持上传和下载两个方向，允许在网络中断后从上次中断的位置继续传输，而不是重新开始。

### 1.1 核心需求

- **上传断点续传**: 客户端查询已有上传进度，从断点位置继续上传
- **下载断点续传**: 客户端查询已有下载进度，从断点位置继续下载
- **分块完整性验证**: 每20MB计算一次分块MD5，确保数据完整性
- **断点有效期**: 24小时自动过期
- **并发控制**: 同一文件同一时间只允许一个上传会话

### 1.2 技术选型

| 项目 | 方案 |
|------|------|
| 断点存储 | Redis Hash |
| 分块大小 | 20MB |
| 完整性校验 | MD5哈希 |
| 过期策略 | 24小时TTL |
| 并发控制 | Redis分布式锁 |

---

## 2. Protocol Buffers 定义

### 2.1 新增消息类型

```protobuf
// 分块信息
message ChunkInfo {
    int32 chunk_index = 1;      // 分块序号（0-based）
    string chunk_md5 = 2;        // 分块的MD5校验值
    int64 chunk_offset = 3;      // 分块在文件中的起始字节偏移
}

// 上传状态响应
message UploadStatus {
    bool has_breakpoint = 1;     // 是否存在断点记录
    int64 resume_offset = 2;     // 建议的续传起始偏移量
    repeated ChunkInfo uploaded_chunks = 3;  // 已上传完成的分块列表
    int64 last_update_time = 4;  // 最后更新时间戳
}

// 下载状态响应
message DownloadStatus {
    bool has_breakpoint = 1;
    int64 resume_offset = 2;     // 服务器记录的已下载位置
    int64 file_size = 3;         // 文件总大小
}

// 上传断点查询请求
message QueryUploadRequest {
    string file_md5 = 1;
}

// 下载断点查询请求
message QueryDownloadRequest {
    string file_name = 1;
    string session_id = 2;       // 客户端会话ID，用于区分不同下载会话
}
```

### 2.2 新增RPC方法

```protobuf
service FileTransport {
    rpc UploadFile(stream UploadRequest) returns (UploadResponse);
    rpc DownloadFile(DownloadRequest) returns (stream DownloadResponse);

    // 新增：断点查询接口
    rpc QueryUploadStatus(QueryUploadRequest) returns (UploadStatus);
    rpc QueryDownloadStatus(QueryDownloadRequest) returns (DownloadStatus);
}
```

### 2.3 修改现有消息

```protobuf
message FileMeta {
    string file_name = 1;
    int64 total_size = 2;
    string file_md5 = 3;
    int64 resume_offset = 4;     // 新增：续传时的起始偏移量（可选）
}

message DownloadRequest {
    string file_name = 1;
    int64 start_offset = 2;      // 新增：从指定偏移量开始下载
}
```

---

## 3. 架构设计

### 3.1 整体架构

```
FileServer
│
├── QueryUploadCallData (断点查询处理器)
│   └── FileRepository.GetUploadProgress()
│       └── Redis: upload_progress:<md5>
│
├── UploadCallData (上传处理)
│   ├── 支持resume_offset参数
│   ├── 验证断点信息
│   └── FileWorker (追加模式写入)
│       ├── 写入磁盘
│       └── 每20MB保存断点
│           ├── FileRepository.UpdateUploadProgress()
│           └── FileRepository.SaveBlockCheckpoint()
│
├── QueryDownloadCallData (断点查询处理器)
│   └── FileRepository.GetDownloadProgress()
│       └── Redis: download_progress:<file>:<session>
│
└── DownloadCallData (下载处理)
    ├── 支持start_offset参数
    ├── seek到指定位置
    ├── 流式发送数据
    └── 每20MB保存断点
        └── FileRepository.UpdateDownloadProgress()
```

### 3.2 核心组件

| 组件 | 职责 | 状态 |
|------|------|------|
| FileRepository | 断点数据CRUD操作 | 已存在，需扩展 |
| UploadCallData | 处理上传RPC请求 | 需修改 |
| DownloadCallData | 处理下载RPC请求 | 需修改 |
| FileWorker | 执行文件I/O操作 | 需修改 |
| QueryUploadCallData | 处理上传断点查询 | 新增 |
| QueryDownloadCallData | 处理下载断点查询 | 新增 |

---

## 4. 数据存储设计

### 4.1 Redis数据结构

#### 上传断点

```
# 主进度信息
Hash: upload_progress:<file_md5>
  - file_name: "example.pdf"
  - total_size: "104857600"
  - uploaded_bytes: "20971520"
  - status: "uploading"
  - updated_at: "1704067200"

# 分块校验信息
Hash: upload_blocks:<file_md5>
  - block_0: "abc123..."
  - block_1: "def456..."
  - block_2: "ghi789..."

TTL: 86400秒 (24小时)
```

#### 下载断点

```
Hash: download_progress:<file_name>:<session_id>
  - file_name: "example.pdf"
  - session_id: "uuid-xxx"
  - downloaded_bytes: "52428800"
  - updated_at: "1704067200"

TTL: 86400秒 (24小时)
```

### 4.2 分块规则

- **分块大小**: 20MB (20 * 1024 * 1024 字节)
- **校验算法**: MD5
- **保存策略**: 每完成一个20MB分块立即保存校验和
- **验证策略**: 续传时验证所有已上传分块的MD5

---

## 5. 上传断点续传流程

### 5.1 完整交互序列

```
客户端                                    服务器
  │                                         │
  │  1. QueryUploadStatus(file_md5)         │
  │───────────────────────────────────────>│
  │                                         │
  │                              2. 查询Redis
  │                              3. 返回断点信息
  │<───────────────────────────────────────│
  │  UploadStatus {                         │
  │    has_breakpoint: true,                │
  │    resume_offset: 20971520,             │
  │    uploaded_chunks: [...]               │
  │  }                                      │
  │                                         │
  │  4. 验证分块MD5                          │
  │  5. UploadFile(stream)                  │
  │     meta: { resume_offset: 20971520 }   │
  │───────────────────────────────────────>│
  │                                         │
  │                              6. 打开文件(追加模式)
  │                                         │
  │  7. chunks[...]                         │
  │───────────────────────────────────────>│
  │                                         │
  │                              8. 写入磁盘
  │                              9. 每20MB保存断点
  │                                         │
  │  10. 继续发送chunks...                  │
  │───────────────────────────────────────>│
  │                                         │
  │  11. 上传完成                           │
  │───────────────────────────────────────>│
  │                              12. 清除断点
  │<───────────────────────────────────────│
  │  UploadResponse { success: true }       │
```

### 5.2 UploadCallData 状态机

```cpp
enum class CallState {
    CREATE,    // 创建调用
    PROCESS,   // 处理元数据
    STREAM,    // 流式传输（新增）
    FINISH     // 完成调用
};
```

**状态转换逻辑:**

```
CREATE → PROCESS: 注册RPC并等待请求
PROCESS → STREAM: 接收到meta，验证断点，开始接收数据
STREAM → STREAM: 接收chunk，写入文件，每20MB保存断点
STREAM → FINISH: 接收完成或错误，清理资源
```

---

## 6. 下载断点续传流程

### 6.1 完整交互序列

```
客户端                                    服务器
  │                                         │
  │  1. QueryDownloadStatus(file_name)      │
  │───────────────────────────────────────>│
  │                                         │
  │                              2. 查询Redis
  │                              3. 获取文件大小
  │                              4. 返回断点信息
  │<───────────────────────────────────────│
  │  DownloadStatus {                       │
  │    has_breakpoint: true,                │
  │    resume_offset: 52428800,             │
  │    file_size: 104857600                 │
  │  }                                      │
  │                                         │
  │  5. DownloadFile { start_offset }       │
  │───────────────────────────────────────>│
  │                                         │
  │                              6. seek到offset
  │                              7. 生成session_id
  │                                         │
  │  8. 流式发送数据                         │
  │<──────────────��────────────────────────│
  │                                         │
  │                              9. 每20MB保存断点
  │                                         │
  │  10. 继续接收...                        │
  │<───────────────────────────────────────│
  │                                         │
  │  11. 下载完成或中断                     │
  │                                         │
  │                              12. 保存最后断点
```

---

## 7. FileRepository 扩展

### 7.1 新增下载断点管理

```cpp
// 下载进度结构
struct DownloadProgress {
    std::string file_name;
    std::string session_id;
    int64 downloaded_bytes;
    int updated_at;
};

// 保存下载断点
static Result<void> SaveDownloadProgress(
    const std::string& file_name,
    const std::string& session_id,
    int64 downloaded_bytes);

// 获取下载断点
static Result<DownloadProgress> GetDownloadProgress(
    const std::string& file_name,
    const std::string& session_id);

// 更新下载断点
static Result<void> UpdateDownloadProgress(
    const std::string& file_name,
    const std::string& session_id,
    int64 downloaded_bytes);

// 删除下载断点
static Result<void> DeleteDownloadProgress(
    const std::string& file_name,
    const std::string& session_id);
```

### 7.2 辅助方法

```cpp
// 获取所有已上传分块列表
static Result<std::vector<ChunkInfo>> GetUploadedChunks(
    const std::string& file_md5,
    int block_size = RESUME_BLOCK_SIZE);

// 获取分块校验和
static Result<std::string> GetBlockCheckpoint(
    const std::string& file_md5,
    int block_index);

// 验证分块完整性
static Result<int> VerifyUploadBlocks(
    const std::string& file_md5,
    int reported_offset,
    int block_size = RESUME_BLOCK_SIZE);
```

### 7.3 常量定义

```cpp
static constexpr int DEFAULT_BLOCK_SIZE = 1 * 1024 * 1024;    // 1MB
static constexpr int RESUME_BLOCK_SIZE = 20 * 1024 * 1024;    // 20MB
static constexpr int DEFAULT_EXPIRE_SECONDS = 24 * 3600;      // 24小时
```

---

## 8. 错误处理与边界情况

### 8.1 上传边界情况

| 场景 | 检测方法 | 处理策略 |
|------|----------|----------|
| 断点文件损坏 | `VerifyUploadBlocks()` MD5不匹配 | 删除断点，返回offset=0 |
| 并发上传同一文件 | 检查status="uploading" | 拒绝请求，返回ABORTED |
| 客户端超时未续传 | Redis TTL | 24小时自动过期 |
| offset超出文件大小 | 比较offset与total_size | 返回INVALID_ARGUMENT |
| 分块不连续 | 检查chunk_index连续性 | 删除断点，重新上传 |

### 8.2 下载边界情况

| 场景 | 检测方法 | 处理策略 |
|------|----------|----------|
| 文件被删除 | `FileIndexManager.find_path()` 返回空 | 返回NOT_FOUND，清除断点 |
| offset超出文件大小 | 比较start_offset与file_size | 返回INVALID_ARGUMENT |
| 多客户端同时下载 | 每个session_id独立 | 允许并发 |
| session_id冲突 | UUID生成 | 服务器生成唯一ID |

---

## 9. 文件修改清单

### 9.1 新增文件

| 文件 | 描述 |
|------|------|
| `servers/FileServer/QueryUploadCallData.h` | 上传断点查询处理器 |
| `servers/FileServer/QueryUploadCallData.cpp` | 上传断点查询实现 |
| `servers/FileServer/QueryDownloadCallData.h` | 下载断点查询处理器 |
| `servers/FileServer/QueryDownloadCallData.cpp` | 下载断点查询实现 |

### 9.2 修改文件

| 文件 | 修改内容 |
|------|----------|
| `proto/file.proto` | 添加断点查询RPC和消息定义 |
| `src/repository/FileRepository.h` | 添加下载断点管理方法声明 |
| `src/repository/FileRepository.cpp` | 实现下载断点管理方法 |
| `servers/FileServer/UploadCallData.h` | 添加resume_offset成员变量 |
| `servers/FileServer/UploadCallData.cpp` | 支持断点续传逻辑 |
| `servers/FileServer/DownloadCallData.h` | 添加session_id和offset成员变量 |
| `servers/FileServer/DownloadCallData.cpp` | 支持start_offset和断点保存 |
| `servers/FileServer/FileWorker.h` | 添加resume_offset到FsTask |
| `servers/FileServer/FileWorker.cpp` | 支持追加模式打开文件和分块MD5计算 |
| `servers/FileServer/CallData.h` | 添加断点查询CallData基类 |
| `servers/FileServer/FileServer.cpp` | 注册新的RPC服务 |
| `servers/FileServer/CMakeLists.txt` | 添加新文件到构建 |

---

## 10. 实施步骤

### 阶段1: Protocol Buffers 和存储层
1. 修改 `proto/file.proto`
2. 重新生成 protobuf 代码
3. 扩展 `FileRepository` 添加下载断点管理

### 阶段2: 断点查询服务
1. 实现 `QueryUploadCallData`
2. 实现 `QueryDownloadCallData`
3. 在 `FileServer` 中注册服务

### 阶段3: 上传断点续传
1. 修改 `FsTask` 添加 resume_offset
2. 修改 `FileWorker` 支持追加模式和分块MD5
3. 修改 `UploadCallData` 支持断点续传

### 阶段4: 下载断点续传
1. 修改 `DownloadCallData` 支持start_offset
2. 实现断点保存逻辑
3. 处理边界情况

### 阶段5: 测试与验证
1. 单元测试
2. 集成测试
3. 性能测试

---

## 11. 测试用例

### 11.1 上传断点续传测试

1. **正常流程**: 上传20MB → 中断 → 查询断点 → 续传完成
2. **断点验证**: 上传40MB → 中断 → 修改磁盘文件 → 续传（应失败）
3. **并发控制**: 两个客户端同时上传同一文件（应拒绝第二个）
4. **过期测试**: 上传20MB → 等待25小时 → 查询断点（应返回空）

### 11.2 下载断点续传测试

1. **正常流程**: 下载20MB → 中断 → 查询断点 → 续传完成
2. **文件删除**: 下载20MB → 中断 → 删除服务器文件 → 续传（应失败）
3. **多并发**: 多个客户端同时下载同一文件（应允许）

---

## 12. 性能考虑

1. **断点保存频率**: 每20MB保存一次，平衡写入频率与数据丢失风险
2. **Redis连接池**: 复用现有连接池（5个连接）
3. **分块MD5计算**: 在FileWorker写入时同步计算，避免二次读取
4. **内存管理**: 使用流式处理，避免大文件全部加载到内存

---

## 13. 安全考虑

1. **MD5校验**: 防止数据损坏和篡改
2. **并发锁**: 防止多个客户端同时修改同一文件
3. **输入验证**: 验证offset、file_size等参数合法性
4. **权限控制**: 确保客户端只能访问自己的断点记录

---

**文档版本历史**
- v1.0 (2026-02-06): 初始设计文档
