# TinyChat 架构文档

## 项目概述

TinyChat 是一个分布式即时通讯系统，采用微服务架构，包含多个独立的后端服务和一个 Qt 桌面客户端。系统支持用户注册、登录、好友管理、实时聊天、文件传输等功能。

## 系统架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                           QTClient (Qt 客户端)                        │
│                    HTTP + TCP 长连接 + gRPC 流式                       │
└─────────────────────────────────────────────────────────────────────┘
                                    │
        ┌───────────────────────────┼───────────────────────────┐
        │                           │                           │
        ▼                           ▼                           ▼
┌───────────────┐          ┌───────────────┐          ┌───────────────┐
│  GateServer   │          │  ChatServer   │          │  FileServer   │
│  (HTTP 网关)   │          │  (TCP + gRPC) │          │   (gRPC)      │
│   Port: 8080  │          │  Port: 8090+  │          │  Port: 50054  │
└───────────────┘          └───────────────┘          └───────────────┘
        │                           │                           │
        │              ┌────────────┴────────────┐              │
        │              │                         │              │
        ▼              ▼                         ▼              ▼
┌───────────────┐  ┌───────────────┐      ┌───────────────┐
│ StatusServer  │  │ VarifyServer  │      │    MySQL      │
│    (gRPC)     │  │   (Node.js)   │      │   Port: 3306  │
│ Port: 50052   │  │  Port: 50051  │      └───────────────┘
└───────────────┘  └───────────────┘              │
        │                  │                      │
        └──────────────────┼──────────────────────┘
                           ▼
                   ┌───────────────┐
                   │    Redis      │
                   │  Port: 6379   │
                   └───────────────┘
```

## 服务器职责

### GateServer (HTTP 网关)

**端口**: 8080  
**协议**: HTTP  
**职责**: 
- 用户认证入口（注册、登录、密码重置）
- 请求路由和负载均衡
- 调用 VarifyServer 发送验证码
- 调用 StatusServer 获取 ChatServer 分配
- 返回 ChatServer 连接信息给客户端

**关键接口**:
| 路由 | 功能 |
|------|------|
| POST /get_varifycode | 获取邮箱验证码 |
| POST /user_register | 用户注册 |
| POST /reset_pwd | 重置密码 |
| POST /user_login | 用户登录 |

### VarifyServer (验证码服务)

**端口**: 50051 (gRPC)  
**技术栈**: Node.js  
**职责**:
- 生成和发送邮箱验证码
- 验证码存储到 Redis（5分钟过期）
- 提供 gRPC 接口给 GateServer 调用

**Proto 定义**:
```protobuf
service VarifyService {
    rpc GetVarifyCode (GetVarifyReq) returns (GetVarifyRsp) {}
}
```

### StatusServer (状态服务)

**端口**: 50052 (gRPC)  
**职责**:
- 管理用户在线状态
- 分配 ChatServer（负载均衡）
- Token 生成和验证
- 用户登录状态同步

**Proto 定义**:
```protobuf
service StatusService {
    rpc GetChatServer (GetChatServerReq) returns (GetChatServerRsp) {}
    rpc Login(LoginReq) returns (LoginRsp);
}
```

### ChatServer (聊天服务)

**端口**: 8090+ (TCP), 50053+ (gRPC)  
**协议**: TCP 长连接 + gRPC  
**职责**:
- 维护客户端 TCP 长连接
- 实时消息转发
- 心跳检测（45秒间隔，60秒超时）
- 好友申请、认证通知
- 消息持久化（通过 gRPC 调用）

**支持可扩展部署**: 配置文件定义了 ChatServer1/2/3，可根据负载动态分配。

**消息类型** (ReqId):
| ReqId | 名称 | 方向 |
|-------|------|------|
| 101 | ID_GET_VARIFY_CODE | 客户端→GateServer |
| 102 | ID_REG_USER | 客户端→GateServer |
| 103 | ID_RESET_PWD | 客户端→GateServer |
| 104 | ID_LOGIN_USER | 客户端→GateServer |
| 105-106 | ID_CHAT_LOGIN_REQ/RSP | 客户端↔ChatServer |
| 107-108 | ID_SEARCH_USER_REQ/RSP | 搜索用户 |
| 109-110 | ID_ADD_FRIEND_REQ/RSP | 添加好友 |
| 117-118 | ID_TEXT_CHAT_MSG_REQ/RSP | 文本消息 |
| 121-122 | ID_HEART_BEAT_REQ/RSP | 心跳 |
| 123-124 | ID_PULL_HISTORY_MSG_REQ/RSP | 主动拉取历史消息 |

### FileServer (文件服务)

**端口**: 50054 (gRPC)  
**协议**: gRPC 双向流  
**职责**:
- 文件上传（客户端流式）
- 文件下载（服务端流式）
- 断点续传支持
- 文件 MD5 校验
- 分块传输管理

**Proto 定义**:
```protobuf
service FileTransport {
  rpc UploadFile(stream UploadRequest) returns (UploadResponse);
  rpc DownloadFile(DownloadRequest) returns (stream DownloadResponse);
  rpc QueryUploadStatus(QueryUploadRequest) returns (UploadStatus);
  rpc QueryDownloadStatus(QueryDownloadRequest) returns (DownloadStatus);
}
```

## 客户端架构 (QTClient)

### 核心管理类

| 类名 | 职责 |
|------|------|
| HttpManager | HTTP 请求管理，处理认证相关请求 |
| TcpManager | TCP 长连接管理，心跳、消息收发 |
| UserManager | 用户信息管理，好友列表缓存 |
| MainWindow | 主窗口，界面状态切换 |

### 通信流程

```
┌─────────────────────────────────────────────────────────────────┐
│                        登录流程                                  │
├─────────────────────────────────────────────────────────────────┤
│ 1. QTClient --HTTP--> GateServer (用户登录)                      │
│ 2. GateServer --gRPC--> StatusServer (获取ChatServer)            │
│ 3. StatusServer 返回 ChatServer 地址 + Token                     │
│ 4. GateServer 返回给 QTClient                                   │
│ 5. QTClient --TCP--> ChatServer (携带 Token 登录)                │
│ 6. ChatServer --gRPC--> StatusServer (验证 Token)                │
│ 7. 登录成功，建立长连接，开始心跳                                  │
│ 8. QTClient 本地先查 SQLite；若为空则发送 123 拉取历史              │
└─────────────────────────────────────────────────────────────────┘
```

## 通信消息格式

### HTTP 消息格式 (客户端 ↔ GateServer)

HTTP 请求/响应均使用 JSON 格式，Content-Type: application/json。

#### 1. 获取验证码

**请求**: POST /get_varifycode
```json
{
    "email": "user@example.com"
}
```

**响应**:
```json
{
    "error": 0,
    "email": "user@example.com"
}
```

#### 2. 用户注册

**请求**: POST /user_register
```json
{
    "email": "user@example.com",
    "user": "username",
    "passwd": "md5_hashed_password",
    "confirm": "md5_hashed_password",
    "varifycode": "123456"
}
```

**响应**:
```json
{
    "error": 0,
    "uid": 1001
}
```

#### 3. 重置密码

**请求**: POST /reset_pwd
```json
{
    "email": "user@example.com",
    "user": "username",
    "passwd": "new_md5_hashed_password",
    "varifycode": "123456"
}
```

**响应**:
```json
{
    "error": 0
}
```

#### 4. 用户登录

**请求**: POST /user_login
```json
{
    "email": "user@example.com",
    "passwd": "md5_hashed_password"
}
```

**响应**:
```json
{
    "error": 0,
    "data": {
        "uid": 1001,
        "host": "127.0.0.1",
        "port": "8090",
        "token": "generated_token_string"
    }
}
```

### TCP 消息格式 (客户端 ↔ ChatServer)

#### 二进制协议

```
┌──────────────────────────────────────────────────────────────┐
│                         消息头 (6字节)                         │
├────────────────────┬─────────────────────┬───────────────────┤
│    Length (4字节)   │   MessageID (2字节)  │    Data (变长)    │
│   Big-Endian       │    Big-Endian       │     UTF-8 JSON    │
└────────────────────┴─────────────────────┴───────────────────┘
```

**字节序**: 大端序 (Big-Endian)  
**消息体**: UTF-8 编码的 JSON 字符串

**发送逻辑** (tcpmanager.cpp:465-479):
```cpp
QByteArray block;
QDataStream out(&block, QIODevice::WriteOnly);
out.setByteOrder(QDataStream::BigEndian);
out << len << id;           // 先写长度(4字节)和ID(2字节)
block.append(body);         // 再追加JSON数据
_socket.write(block);
```

**接收逻辑** (tcpmanager.cpp:332-355):
```cpp
const int HEADER_SIZE = 6;
QDataStream headStream(_buffer);
headStream.setByteOrder(QDataStream::BigEndian);
quint32 message_len;
quint16 message_id;
headStream >> message_len >> message_id;  // 读取头
QByteArray messageBody = _buffer.mid(HEADER_SIZE, message_len);
```

#### 消息类型详解

##### ID_CHAT_LOGIN_REQ (105) - 登录聊天服务器

**请求**:
```json
{
    "uid": 1001,
    "token": "generated_token_string"
}
```

**响应 ID_CHAT_LOGIN_RSP (106)**:
```json
{
    "error": 0,
    "uid": 1001,
    "name": "用户名",
    "nick": "昵称",
    "icon": "头像路径",
    "sex": 1,
    "token": "token",
    "apply_list": [...],      // 好友申请列表
    "friend_list": [...]       // 好友列表
}
```

说明：登录回包不再直接携带 `recent_messages`。历史消息改为客户端在本地缓存为空时，通过 `ID_PULL_HISTORY_MSG_REQ(123)` 主动请求。

##### ID_SEARCH_USER_REQ (107) - 搜索用户

**请求**:
```json
{
    "uid": 1001,
    "name": "搜索的用户名"
}
```

**响应 ID_SEARCH_USER_RSP (108)**:
```json
{
    "error": 0,
    "uid": 1002,
    "name": "用户名",
    "nick": "昵称",
    "desc": "个性签名",
    "icon": "头像路径",
    "sex": 1
}
```

##### ID_ADD_FRIEND_REQ (109) - 添加好友申请

**请求**:
```json
{
    "applyuid": 1001,
    "touid": 1002,
    "name": "申请人名称",
    "desc": "申请说明",
    "icon": "申请人头像",
    "nick": "申请人昵称",
    "sex": 1
}
```

**响应 ID_ADD_FRIEND_RSP (110)**:
```json
{
    "error": 0,
    "applyuid": 1001,
    "touid": 1002
}
```

##### ID_NOTIFY_ADD_FRIEND_REQ (111) - 通知被申请者

**服务器推送**:
```json
{
    "error": "success",
    "applyuid": 1001,
    "name": "申请人名称",
    "desc": "申请说明",
    "icon": "申请人头像",
    "nick": "申请人昵称",
    "sex": 1
}
```

##### ID_AUTH_FRIEND_REQ (113) - 认证好友申请

**请求** (同意/拒绝好友申请):
```json
{
    "fromuid": 1001,
    "touid": 1002,
    "agree": true
}
```

**响应 ID_AUTH_FRIEND_RSP (114)**:
```json
{
    "error": 0,
    "uid": 1002,
    "name": "好友名称",
    "nick": "好友昵称",
    "icon": "好友头像",
    "sex": 1
}
```

##### ID_NOTIFY_AUTH_FRIEND_REQ (115) - 通知好友认证结果

**服务器推送**:
```json
{
    "error": 0,
    "fromuid": 1001,
    "name": "好友名称",
    "nick": "好友昵称",
    "icon": "好友头像",
    "sex": 1
}
```

##### ID_TEXT_CHAT_MSG_REQ (117) - 发送文本消息

**请求**:
```json
{
    "fromuid": 1001,
    "touid": 1002,
    "text_array": [
        {
            "msgid": "uuid-string-1",
            "content": "消息内容1"
        },
        {
            "msgid": "uuid-string-2",
            "content": "消息内容2"
        }
    ]
}
```

**响应 ID_TEXT_CHAT_MSG_RSP (118)**:
```json
{
    "error": 0,
    "fromuid": 1001,
    "touid": 1002
}
```

##### ID_NOTIFY_TEXT_CHAT_MSG_REQ (119) - 通知接收消息

**服务器推送**:
```json
{
    "error": 0,
    "fromuid": 1001,
    "touid": 1002,
    "text_array": [
        {
            "msgid": "uuid-string",
            "content": "消息内容"
        }
    ]
}
```

##### ID_NOTIFY_OFF_LINE_REQ (120) - 通知下线

**服务器推送**:
```json
{
    "error": 0
}
```

##### ID_HEART_BEAT_REQ (121) - 心跳请求

**请求**:
```json
{
    "error": 0,
    "timestamp": 1700000000
}
```

**响应 ID_HEARTBEAT_RSP (122)**:
```json
{
    "error": 0
}
```

**心跳探测** (服务器主动探测):
```json
{
    "probe": true
}
```

##### ID_PULL_HISTORY_MSG_REQ (123) - 客户端拉取历史消息

**请求**:
```json
{
    "uid": 1001,
    "days": 7,
    "limit": 50
}
```

说明：客户端仅在本地 SQLite 缓存为空时发送该请求。

##### ID_PULL_HISTORY_MSG_RSP (124) - 历史消息回包

**响应**:
```json
{
    "error": 0,
    "uid": 1001,
    "messages": [
        {
            "fromuid": 1001,
            "touid": 1002,
            "text_array": [
                {
                    "msgid": "uuid-string",
                    "content": "消息内容"
                }
            ]
        }
    ]
}
```

### gRPC 消息格式 (服务间通信)

#### FileServer 文件传输

**上传流式消息 UploadRequest**:
```protobuf
message UploadRequest {
  oneof data {
    FileMeta meta = 1;   // 首条消息：文件元数据
    bytes chunk = 2;     // 后续消息：文件数据块
  }
}

message FileMeta {
  string file_name = 1;
  int64 total_size = 2;
  string file_md5 = 3;
  int64 resume_offset = 4;  // 断点续传偏移
}
```

**下载流式消息 DownloadResponse**:
```protobuf
message DownloadResponse {
  oneof data {
    FileMeta meta = 1;   // 首条消息：文件元数据
    bytes chunk = 2;     // 后续消息：文件数据块
  }
}
```

### 错误码定义

| 错误码 | 名称 | 说明 |
|--------|------|------|
| 0 | SUCCESS | 成功 |
| 1 | ERROR_JSON | JSON 解析错误 |
| 2 | ERROR_NETWORK | 网络错误 |
| 1001 | ERROR_JSON | JSON 解析错误 |
| 1002 | RPC_FAILED | RPC 请求失败 |
| 1003 | VARIFY_EXPIRED | 验证码过期 |
| 1004 | VARIFY_CODE_ERROR | 验证码错误 |
| 1005 | USER_EXIST | 用户已存在 |
| 1006 | PASSWORD_ERROR | 密码错误 |
| 1007 | EMAIL_NOT_MATCH | 邮箱不匹配 |
| 1014 | UID_INVALID | UID 无效 |
| 1015 | TOKEN_INVALID | Token 无效 |

## 数据存储

### MySQL (TinyChat 数据库)

主要表：
- `user`: 用户信息（uid, name, email, pwd, icon, sex, nick, desc, back）
- `friend`: 好友关系
- `apply`: 好友申请记录
- `message`: 聊天消息持久化

### Redis

用途：
- 验证码存储（key: email, value: code, TTL: 300s）
- 用户 Token 存储
- ChatServer 在线状态
- 用户登录状态缓存
- 分布式锁

### SQLite (QTClient 本地缓存)

用途：
- 客户端本地文本消息缓存（按当前登录 uid 隔离）
- 登录后执行“本地优先，远端补齐”策略：本地有记录不拉取，本地空再请求 123

缓存文件路径：
- `QTClient/chat_cache.db`

## 关键数据结构

### 用户相关 (userdata.h)

```cpp
struct UserInfo {
    int _uid;
    QString _name, _nick, _icon, _desc, _last_msg;
    int _sex;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
};

struct FriendInfo {
    int _uid;
    QString _name, _nick, _icon, _desc, _back, _last_msg;
    int _sex;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
};

struct TextChatData {
    QString _msg_id, _msg_content;
    int _from_uid, _to_uid;
};
```

### 消息气泡角色

```cpp
enum class ChatRole {
    SELF,   // 当前用户消息（右侧，绿色气泡）
    OTHER,  // 对方消息（左侧，白色气泡）
};
```

## 目录结构

```
TinyChat/
├── Backend/
│   ├── servers/
│   │   ├── GateServer/      # HTTP 网关
│   │   ├── VarifyServer/    # Node.js 验证码服务
│   │   ├── StatusServer/    # gRPC 状态服务
│   │   ├── ChatServer/      # TCP + gRPC 聊天服务
│   │   └── FileServer/      # gRPC 文件服务
│   ├── src/                  # 共享库
│   │   ├── core/            # HTTP 核心逻辑
│   │   ├── infra/           # 基础设施（连接池、日志、配置）
│   │   ├── dao/             # 数据访问对象
│   │   ├── repository/      # 数据仓储层
│   │   ├── service/         # 业务服务层
│   │   ├── controller/      # HTTP 控制器
│   │   ├── grpcClient/      # gRPC 客户端
│   │   └── common/          # 公共类型
│   ├── proto/               # Protobuf 定义
│   ├── sql/                 # 数据库迁移脚本
│   └── config.ini           # 服务配置
│
└── QTClient/                 # Qt 客户端
    ├── *.ui                  # Qt Designer 界面文件
    ├── *.h/*.cpp             # C++ 源码
    ├── resources.qrc         # 资源文件
    └── QTClient.pro          # qmake 项目文件
```

## 配置说明

配置文件 `Backend/config.ini` 定义了所有服务端口和数据库连接：

```ini
[GateServer]
host = 127.0.0.1
port = 8080

[ChatServer1]
host = 127.0.0.1
port = 8090           # TCP 端口
RPCport = 50053       # gRPC 端口
heartbeat_interval = 45
heartbeat_timeout = 60

[Redis]
host = 127.0.0.1
port = 6379

[MySQL]
host = 127.0.0.1
schema = TinyChat
```

## 构建命令

```bash
# 后端构建
cd Backend && mkdir build && cd build
cmake .. && make -j$(nproc)

# 客户端构建
cd QTClient
qmake && make
```

## 启动顺序

1. Redis 服务
2. MySQL 服务
3. VarifyServer (Node.js)
4. StatusServer
5. ChatServer (可启动多个实例)
6. FileServer
7. GateServer
8. QTClient
