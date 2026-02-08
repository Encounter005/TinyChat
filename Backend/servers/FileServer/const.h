#ifndef FILE_CONST_H_
#define FILE_CONST_H_

#include "file.grpc.pb.h"
#include "file.pb.h"
#include "infra/LogManager.h"
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <grpcpp/completion_queue.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/sync_stream.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

using FileService::DownloadRequest;
using FileService::DownloadResponse;
using FileService::FileMeta;
using FileService::FileTransport;
using FileService::QueryDownloadRequest;
using FileService::QueryUploadRequest;
using FileService::UploadRequest;
using FileService::UploadResponse;
using grpc::Server;
using grpc::ServerAsyncReader;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerAsyncWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::Status;

using int64 = int64_t;


enum class TaskType {
    META,   // 文件元数据（打开文件）
    DATA,   // 文件数据块（写入文件）
    END,    // 结束标志（关闭文件）
};

enum class CallState {
    CREATE,    // 创建调用
    PROCESS,   // 处理请求
    STREAM,
    FINISH,   // 完成调用
};

// 断点查询确认
struct ResumeValidationResult {
    bool        should_allow_upload;         // 是否允许上传
    bool        should_clear_old_progress;   // 是否清除旧断点
    std::string reason;
};


// 下载验证
struct DownloadValidationResult {
    bool             is_valid;
    std::string      file_path;
    int64            file_size;
    grpc::StatusCode error_code;
    std::string      error_message;
};

// 下载会话信息
struct DownloadSessionInfo {
    std::string session_id;
    std::string file_path;
    int64       start_offset;
    int64       current_offset;
    int64       file_size;
};

// 性能监控统计
struct DownloadPerformanceMetrics {
    std::string session_id;
    std::string file_name;

    double file_open_duration_ms     = 0;
    double file_seek_duration_ms     = 0;
    double file_read_duration_ms     = 0;
    double network_write_duration_ms = 0;
    double total_duration_ms         = 0;

    // IO
    int64 bytes_sent  = 0;
    int   chunk_count = 0;

    double avg_read_speed_mb_s() const {
        if (file_read_duration_ms <= 0.0) return 0.0;
        double total_mb      = bytes_sent / (1024.0 * 1024.0);
        double total_seconds = file_read_duration_ms / 1000.0;
        return total_mb / total_seconds;
    }

    double avg_read_duration_ms() const {
        if (chunk_count <= 0) return 0.0;
        return file_read_duration_ms / chunk_count;
    }

    double avg_write_duration_ms() const {
        if (chunk_count <= 0) return 0.0;
        return network_write_duration_ms / chunk_count;
    }
    void log_summary() const {

        LOG_INFO(
            "========== Download Performance Summary ==========\n"
            "  Session ID:      {}\n"
            "  File Name:       {}\n"
            "  Bytes Sent:      {} bytes ({:.2f} MB)\n"
            "  Chunk Count:     {}\n"
            "  Total Duration:  {:.2f} ms ({:.2f} s)\n"
            "----------------------------------------\n"
            "  File Open:       {:.2f} ms\n"
            "  File Seek:       {:.2f} ms\n"
            "  File Read:       {:.2f} ms (avg: {:.2f} ms/chunk)\n"
            "  Network Write:   {:.2f} ms (avg: {:.2f} ms/chunk)\n"
            "----------------------------------------\n"
            "  Avg Read Speed:  {:.2f} MB/s\n"
            "  Throughput:      {:.2f} MB/s (total)\n"
            "==========================================",
            session_id,
            file_name,
            bytes_sent,
            bytes_sent / (1024.0 * 1024.0),
            chunk_count,
            total_duration_ms,
            total_duration_ms / 1000.0,
            file_open_duration_ms,
            file_seek_duration_ms,
            file_read_duration_ms,
            avg_read_duration_ms(),
            network_write_duration_ms,
            avg_write_duration_ms(),
            avg_read_speed_mb_s(),
            (bytes_sent / (1024.0 * 1024.0)) / (total_duration_ms / 1000.0));
    }
};


#endif   // CONST_H_
