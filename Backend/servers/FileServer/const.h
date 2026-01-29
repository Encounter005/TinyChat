#ifndef CONST_H_
#define CONST_H_

#include "file.grpc.pb.h"
#include "file.pb.h"
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <grpcpp/completion_queue.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/async_stream.h>
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
using FileService::UploadRequest;
using FileService::UploadResponse;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpc::ServerReader;
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncReader;
using grpc::ServerCompletionQueue;


enum class TaskType {
    META, // 文件元数据（打开文件）
    DATA, // 文件数据块（写入文件）
    END, // 结束标志（关闭文件）
};

enum class CallState {
    CREATE,      // 创建调用
    PROCESS,     // 处理请求
    FINISH,      // 完成调用
    FINISH_DONE  // 完成（待删除）
};

// Session
struct UploadSession {
    std::string file_md5;
    std::string file_name;
    std::unique_ptr<ServerAsyncWriter<UploadResponse>> writer;
    std::atomic<int> pending_chunks{0};
    std::condition_variable cv;
    bool completed = false;
};

#endif   // CONST_H_
