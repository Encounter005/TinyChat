#include "UploadCallData.h"
#include "BackPressureManager.h"
#include "FileWorkerPool.h"
#include "TaskQueue.h"
#include "const.h"
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
    if (!ok) {
        // client 已发送完毕
        _state = CallState::FINISH;

        FileWorkerPool::getInstance()->Submit(FsTask::createEnd(_current_md5));

        _response.set_success(true);
        _response.set_message("OK");

        BackPressureManager::getInstance()->notify_all();

        _reader.Finish(_response, grpc::Status::OK, this);
        return;
    }
    // 启用备压
    applyBackPressure();

    createNextHandler();


    if (_request.has_meta()) {
        handleMetaMessage();
    } else if (_request.has_chunk()) {
        handleChunkMessage();
    }

    // 继续读
    _reader.Read(&_request, this);
}
void UploadCallData::handleFinishState() {}

// 消息处理
void UploadCallData::handleMetaMessage() {
    const auto& meta  = _request.meta();
    _current_md5      = meta.file_md5();
    _current_filename = meta.file_name();
    _resume_offset    = meta.resume_offset();   // 获取偏移量

    processResumeLogic(meta);

    LOG_INFO(
        "[UploadCallData] Upload request: file={}, md5={}, "
        "resume_offset={}",
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
    result.should_clear_old_progress = true;

    auto exists_result = FileRepository::ExistsUploadProgress(md5);
    if (!exists_result.IsOK() || !exists_result.Value()) {
        // 没有记录可以直接传输
        return result;
    }

    auto progress_result = FileRepository::GetUploadProgress(md5);
    if (!progress_result.IsOK()) {
        result.should_clear_old_progress = true;
        return result;
    }

    auto progress = progress_result.Value();

    // 1. 有正在进行的传输
    if (progress.status == "uploading" && progress.uploaded_bytes > 0) {
        // 新请求
        if (requested_offset == 0) {
            result.should_clear_old_progress = true;
            result.reason = "New upload request, clearning old progresss";
        } else {
            // 续传请求
            result.reason = "Resume upload with offset="
                            + std::to_string(requested_offset);
        }
        return result;
    }

    // 2. 无效断点(offset)，自动清除
    if (progress.uploaded_bytes == 0 || progress.status != "uploading") {
        LOG_INFO(
            "[UploadCallData] Clearing stale breakpoint for md5: {}, (offset = "
            "{}, status = {})",
            md5,
            progress.uploaded_bytes,
            progress.status);

        result.should_clear_old_progress = true;
        result.reason                    = "Stale progress (offset = "
                        + std::to_string(progress.uploaded_bytes)
                        + ", status = " + progress.status + ")";
    }

    return result;
}

void UploadCallData::processResumeLogic(const FileService::FileMeta& meta) {
    // 验证断点续传请求
    auto validation = validateResumeRequest(_current_md5, _resume_offset);

    // 清除旧的断点续传记录
    if (validation.should_clear_old_progress) {
        FileRepository::DeleteUploadProgress(_current_md5);
        FileRepository::DeleteBlockCheckpoints(_current_md5);
        LOG_INFO("[UploadCallData] Cleared old progress {}", validation.reason);
    }

    // 如果是新上传(offset = 0)
    if (_resume_offset == 0) {
        FileRepository::DeleteUploadProgress(_current_md5);
        FileRepository::DeleteBlockCheckpoints(_current_md5);
    }
    FileRepository::SaveUploadProgress(
        _current_md5, _current_filename, 0, _resume_offset, "uploading");
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
