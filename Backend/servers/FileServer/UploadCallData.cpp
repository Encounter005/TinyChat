#include "UploadCallData.h"
#include "BackPressureManager.h"
#include "FileWorkerPool.h"
#include "TaskQueue.h"
#include "const.h"
#include "file.grpc.pb.h"
#include "file.pb.h"
#include "infra/LogManager.h"
#include "repository/FileRepository.h"
#include <grpcpp/support/status.h>
#include <string>


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
    switch (_state) {
    case CallState::CREATE: handleCreateState(); break;
    case CallState::PROCESS: handleProcessState(ok); break;
    case CallState::FINISH: handleFinishState(); break;
    default: cleanup(); break;
    }
}


void UploadCallData::handleCreateState() {
    _state = CallState::PROCESS;
    // 注册 RPC
    _service->RequestUploadFile(&_ctx, &_reader, _cq, _cq, this);
}

void UploadCallData::handleProcessState(bool ok) {
    // 第一次进入 PROCESS：RPC 被 accept
    if (!_stream_started) {
        if (!ok) {
            // gRPC 正在 shutdown 时，Request* 可能直接返回 ok=false
            // 这里不能再调用 Finish，直接释放即可
            cleanup();
            return;
        }

        _stream_started = true;
        if (!_spawned_next_handler) {
            createNextHandler();   // 每个 RPC 只创建一次下一处理器
            _spawned_next_handler = true;
        }
        _reader.Read(&_request, this);
        return;
    }

    // 后续 Read 回调
    if (!ok) {
        // 注意：async API 下此处不能调用 _ctx.IsCancelled()
        bool stream_ok = _meta_received && !_has_error;
        if (stream_ok && _expected_total_size > 0) {
            const int64 final_uploaded
                = _resume_offset + _received_stream_bytes;
            if (final_uploaded != _expected_total_size) {
                stream_ok      = false;
                _error_message = "uploaded size mismatch";
            }
        }

        if (_meta_received && !_finalize_submitted) {
            FileWorkerPool::getInstance()->Submit(
                FsTask::createEnd(_current_md5, stream_ok));
            _finalize_submitted = true;
        }

        BackPressureManager::getInstance()->notify_all();
        _state = CallState::FINISH;

        if (stream_ok) {
            _response.set_success(true);
            _response.set_message("OK");
            _reader.Finish(_response, grpc::Status::OK, this);
        } else {
            const std::string reason
                = _error_message.empty() ? "upload terminated before completion"
                                         : _error_message;
            _response.set_success(false);
            _response.set_message(reason);
            _reader.Finish(
                _response,
                grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, reason),
                this);
        }
        return;
    }

    if (_request.has_meta()) {
        if (_meta_received) {
            _has_error     = true;
            _error_message = "duplicate meta message";
        } else {
            handleMetaMessage();
        }
    } else if (_request.has_chunk()) {
        if (!_meta_received) {
            _has_error     = true;
            _error_message = "chunk before meta";
        } else {
            applyBackPressure();
            _received_stream_bytes
                += static_cast<int64>(_request.chunk().size());
            handleChunkMessage();
        }
    } else {
        _has_error     = true;
        _error_message = "unknown upload payload";
    }

    if (_has_error) {
        if (_meta_received && !_finalize_submitted) {
            FileWorkerPool::getInstance()->Submit(
                FsTask::createEnd(_current_md5, false));
            _finalize_submitted = true;
        }

        BackPressureManager::getInstance()->notify_all();
        _state = CallState::FINISH;
        _response.set_success(false);
        _response.set_message(_error_message);
        _reader.Finish(
            _response,
            grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, _error_message),
            this);
        return;
    }

    _reader.Read(&_request, this);
}

void UploadCallData::handleFinishState() {
    cleanup();
}

// 消息处理
void UploadCallData::handleMetaMessage() {
    const auto& meta = _request.meta();

    _current_md5         = meta.file_md5();
    _current_filename    = meta.file_name();
    _resume_offset       = meta.resume_offset();
    _expected_total_size = meta.total_size();
    _meta_received       = true;

    if (_current_md5.empty() || _current_filename.empty()) {
        _has_error     = true;
        _error_message = "invalid meta: empty file name or md5";
        return;
    }

    if (_expected_total_size <= 0) {
        _has_error     = true;
        _error_message = "invalid meta: total_size <= 0";
        return;
    }

    if (_resume_offset < 0 || _resume_offset > _expected_total_size) {
        _has_error     = true;
        _error_message = "invalid meta: resume_offset out of range";
        return;
    }

    if (!processResumeLogic(meta)) {
        _has_error = true;
        if (_error_message.empty()) {
            _error_message = "resume validation failed";
        }
        return;
    }

    LOG_INFO(
        "[UploadCallData] Upload request: file={}, md5={}, resume_offset={}",
        _current_filename,
        _current_md5,
        _resume_offset);

    FileWorkerPool::getInstance()->Submit(
        FsTask::createMeta(_current_filename, _current_md5, _resume_offset));
}

void UploadCallData::handleChunkMessage() {

    FileWorkerPool::getInstance()->Submit(
        FsTask::createData(_current_md5, std::move(*_request.mutable_chunk())));
}

ResumeValidationResult UploadCallData::validateResumeRequest(
    const std::string& md5, int64 requested_offset) {


    ResumeValidationResult result;
    result.should_allow_upload       = true;
    result.should_clear_old_progress = false;
    result.reason.clear();

    if (requested_offset == 0) {
        result.should_clear_old_progress = true;
        result.reason                    = "fresh upload";
        return result;
    }

    auto progress_result = FileRepository::GetUploadProgress(md5);
    if (!progress_result.IsOK()) {
        result.should_allow_upload = false;
        result.reason              = "resume requested but no progress found";
        return result;
    }

    const auto progress = progress_result.Value();

    if (progress.status == "completed") {
        result.should_allow_upload = false;
        result.reason              = "upload already completed";
        return result;
    }

    auto verified_offset_result = FileRepository::VerifyUploadBlocks(
        md5, progress.uploaded_bytes, FileRepository::RESUME_BLOCK_SIZE);

    if (!verified_offset_result.IsOK()) {
        result.should_allow_upload = false;
        result.reason              = "failed to verify upload blocks";
        return result;
    }

    const int64 verified_offset = verified_offset_result.Value();

    if (requested_offset != verified_offset) {
        result.should_allow_upload = false;
        result.reason              = "resume offset mismatch, requested="
                        + std::to_string(requested_offset)
                        + ", server=" + std::to_string(verified_offset);
        return result;
    }

    result.reason = "resume accepted";
    return result;
}

bool UploadCallData::processResumeLogic(const FileService::FileMeta& meta) {
    auto validation = validateResumeRequest(_current_md5, _resume_offset);

    if (!validation.should_allow_upload) {
        _error_message = validation.reason;
        return false;
    }

    if (validation.should_clear_old_progress) {
        FileRepository::DeleteUploadProgress(_current_md5);
        FileRepository::DeleteBlockCheckpoints(_current_md5);
        LOG_INFO(
            "[UploadCallData] Cleared old progress: {}", validation.reason);
    }

    auto save_result = FileRepository::SaveUploadProgress(
        _current_md5,
        _current_filename,
        meta.total_size(),
        _resume_offset,
        "uploading");

    if (!save_result.IsOK()) {
        _error_message = "failed to save upload progress";
        return false;
    }

    return true;
}

void UploadCallData::createNextHandler() {
    new UploadCallData(_service, _cq);
}

void UploadCallData::applyBackPressure() {
    if (_request.has_chunk()) {
        BackPressureManager::getInstance()->wait_if_needed(
            _request.chunk().size());
    }
}

void UploadCallData::cleanup() {
    delete this;
}
