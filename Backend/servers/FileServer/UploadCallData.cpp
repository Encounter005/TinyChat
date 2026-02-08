#include "UploadCallData.h"
#include "BackPressureManager.h"
#include "FileWorkerPool.h"
#include "TaskQueue.h"
#include "const.h"
#include "file.pb.h"
#include "infra/LogManager.h"
#include "repository/FileRepository.h"
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

// CREATE -> PROCESSS (with resume) -> STREAM -> FINISH
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

            BackPressureManager::getInstance()->notify_all();

            _reader.Finish(_response, grpc::Status::OK, this);
            return;
        }


        if (_request.has_chunk()) {
            BackPressureManager::getInstance()->wait_if_needed(
                _request.chunk().size());
        }

        new UploadCallData(_service, _cq);

        if (_request.has_meta()) {
            const auto& meta  = _request.meta();
            _current_md5      = meta.file_md5();
            _current_filename = meta.file_name();
            _resume_offset    = meta.resume_offset();   // 获取偏移量


            LOG_INFO(
                "[UploadCallData] Upload request: file={}, md5={}, "
                "resume_offset={}",
                _current_filename,
                _current_md5,
                _resume_offset);

            // 检查是否有旧的上传记录（并发控制 + 断点清理）
            auto exists_result
                = FileRepository::ExistsUploadProgress(_current_md5);
            if (exists_result.IsOK() && exists_result.Value()) {
                auto progress_result
                    = FileRepository::GetUploadProgress(_current_md5);
                if (progress_result.IsOK()) {
                    auto progress = progress_result.Value();

                    // 情况1: 有正在进行的上传（offset > 0），拒绝新的上传请求
                    if (progress.status == "uploading"
                        && progress.uploaded_bytes > 0) {
                        if (_resume_offset == 0) {

                            FileRepository::DeleteUploadProgress(_current_md5);
                            FileRepository::DeleteBlockCheckpoints(
                                _current_md5);
                        }
                    }

                    // 情况2:
                    // 无效断点（offset=0）或旧的上传记录，自动清除并允许重新上传
                    if (progress.uploaded_bytes == 0
                        || progress.status != "uploading") {
                        LOG_INFO(
                            "[UploadCallData] Clearing stale breakpoint for "
                            "md5: {} "
                            "(offset={}, status={})",
                            _current_md5,
                            progress.uploaded_bytes,
                            progress.status);
                        FileRepository::DeleteUploadProgress(_current_md5);
                        FileRepository::DeleteBlockCheckpoints(_current_md5);
                    }
                }
            }

            // 保存或更新上传进度
            // 如果是新上传(offset=0)，先清除旧记录再创建
            // 如果是续传(offset>0)，更新已有记录
            if (_resume_offset == 0) {
                FileRepository::DeleteUploadProgress(_current_md5);
                FileRepository::DeleteBlockCheckpoints(_current_md5);
            }

            FileRepository::SaveUploadProgress(
                _current_md5,
                _current_filename,
                0,                // total_size 未知
                _resume_offset,   // 当前已上传字节数（resume_offset）
                "uploading");

            FileWorkerPool::getInstance()->Submit(
                FsTask::createMeta(
                    _current_filename, _current_md5, _resume_offset));
        } else if (_request.has_chunk()) {

            FileWorkerPool::getInstance()->Submit(
                FsTask::createData(
                    _current_md5, std::move(*_request.mutable_chunk())));
        }

        // 继续读
        _reader.Read(&_request, this);

    } else {
        delete this;
    }
}
