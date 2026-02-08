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
        void handleMetaMessage();
        void handleChunkMessage();
        ResumeValidationResult validateResumeRequest(const std::string& md5, int64 requested_offset);
        void processResumeLogic(const FileService::FileMeta& meta);

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
};

#endif   // UPLOADCALLDATA_H_
