#include "UploadCallData.h"
#include "FileWorkerPool.h"
#include "TaskQueue.h"
#include "const.h"
#include "file.pb.h"
#include "infra/LogManager.h"
#include <grpcpp/support/status.h>

UploadCallData::UploadCallData(
    FileTransport::AsyncService* service, grpc::ServerCompletionQueue* cq)
    : _service(service)
    , _cq(cq)
    , _ctx()
    , _reader(&_ctx)
    , _state(CallState::CREATE) {
    Proceed(true);
}

void UploadCallData::Proceed(bool ok) {
    if (_state == CallState::CREATE) {
        _state = CallState::PROCESS;

        // 注册 RPC
        _service->RequestUploadFile(&_ctx, &_reader, _cq, _cq, this);


    } else if (_state == CallState::PROCESS) {

        if (!ok) {
            // client 已发送完毕
            _state = CallState::FINISH;

            FileWorkerPool::getInstance()->Submit(
                FsTask::createEnd(_current_md5));

            _response.set_success(true);
            _response.set_message("OK");

            _reader.Finish(_response, grpc::Status::OK, this);
            return;
        }

        new UploadCallData(_service, _cq);

        if (_request.has_meta()) {
            const auto& meta  = _request.meta();
            _current_md5      = meta.file_md5();
            _current_filename = meta.file_name();

            FileWorkerPool::getInstance()->Submit(
                FsTask::createMeta(_current_filename, _current_md5));
        } else if (_request.has_chunk()) {
            FileWorkerPool::getInstance()->Submit(
                FsTask::createData(_current_md5, _request.chunk()));
        }

        // 继续读
        _reader.Read(&_request, this);

    } else {
        delete this;
    }
}
