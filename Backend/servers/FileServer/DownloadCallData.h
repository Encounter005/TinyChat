#ifndef DOWNLOADCALLDATA_H_
#define DOWNLOADCALLDATA_H_

#include "CallData.h"
#include "const.h"
#include "file.grpc.pb.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <grpcpp/support/status.h>
#include <vector>

class DownloadCallData : public CallData {
public:
    DownloadCallData(
        FileService::FileTransport::AsyncService* service,
        grpc::ServerCompletionQueue*              cq);
    void Proceed(bool ok) override;

private:
    bool initializeDownloadSession(
        const std::string& filename, const std::string& file_path,
        int64_t start_offset, int64_t file_size);

    void handleCreateState();
    void handleProcessState(bool ok);
    void handleStreamState(bool ok);
    void handleFinishState();

    DownloadValidationResult validateDownloadRequest(
        const std::string& file_name, int64 start_offset);

    // file operation
    bool openFileStream(const std::string& file_path, int64 start_offset);
    void sendFileMetadata(const std::string& file_name, int64 file_size);

    // stream handle
    void readAndSendChunk();
    bool shouldSaveBreakPoint() const;
    void saveDownloadBreakPoint();
    void handleDownloadComplete();
    void handleDownloadCancelled();

    // ErrorHandle
    void finishWithError(grpc::StatusCode code, const std::string& message);
    void handleFileNotFoundError(const std::string& file_name);
    void handleFileSizeError();
    void handleInvalidOffsetError(int64 offset, int64 file_size);
    void handleFileOpenError();


    // @brief:  生成会话ID
    std::string GenerateSessionID();
    void        createNextHandler();
    void        cleanup();

private:
    FileService::FileTransport::AsyncService*              _service;
    grpc::ServerCompletionQueue*                           _cq;
    grpc::ServerContext                                    _context;
    FileService::DownloadRequest                           _request;
    grpc::ServerAsyncWriter<FileService::DownloadResponse> _writer;
    CallState                                              _state;
    std::ifstream                                          _file_stream;
    std::vector<char>                                      _buffer;

    std::string _session_id;           // 下载会话ID
    int64_t     _current_offset = 0;   // 当前发送位置
    int64_t     _start_offset   = 0;   // 起始偏移量
    int64_t     _file_size      = 0;   // 文件总大小

    DownloadPerformanceMetrics                     _metrics;
    std::chrono::high_resolution_clock::time_point _start_time;

    static constexpr int64_t SAVE_BREAKPOINT_INTERVAL
        = 20 * 1024 * 1024;   // 20MB
    static constexpr size_t CHUNK_SIZE = 2 * 1024 * 1024;
};

#endif   // DOWNLOADCALLDATA_H_
