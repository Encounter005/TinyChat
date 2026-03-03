#ifndef UPLOADCALLDATA_H_
#define UPLOADCALLDATA_H_

#include "CallData.h"
#include "const.h"
#include "file.grpc.pb.h"
#include "file.pb.h"
#include "repository/FileRepository.h"
#include <grpcpp/completion_queue.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>


class UploadCallData : public CallData {
public:
    UploadCallData(
        FileService::FileTransport::AsyncService* service,
        grpc::ServerCompletionQueue*              cq);
    ~UploadCallData() = default;

    void Proceed(bool ok) override;


private:
    void handleCreateState();
    void handleProcessState(bool ok);
    void handleFinishState();

    // 消息处理
    void                   handleMetaMessage();
    void                   handleChunkMessage();
    ResumeValidationResult validateResumeRequest(
        const std::string& md5, int64 requested_offset);
    bool processResumeLogic(const FileService::FileMeta& meta);

    void createNextHandler();
    void applyBackPressure();
    void cleanup();

private:
    FileService::FileTransport::AsyncService* _service;
    grpc::ServerCompletionQueue*              _cq;

    grpc::ServerContext _ctx;
    grpc::ServerAsyncReader<
        FileService::UploadResponse, FileService::UploadRequest>
        _reader;

    FileService::UploadRequest  _request;
    FileService::UploadResponse _response;

    CallState   _state;
    std::string _current_md5;
    std::string _current_filename;
    int64       _resume_offset = 0;

    bool        _stream_started        = false;
    bool        _spawned_next_handler  = false;
    bool        _meta_received         = false;
    bool        _has_error             = false;
    bool        _finalize_submitted    = false;
    int64       _expected_total_size   = 0;
    int64       _received_stream_bytes = 0;
    std::string _error_message;
};

#endif   // UPLOADCALLDATA_H_
