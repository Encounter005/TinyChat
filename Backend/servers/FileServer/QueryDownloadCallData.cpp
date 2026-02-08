#include "QueryDownloadCallData.h"

#include "FileIndexManager.h"
#include "QueryDownloadCallData.h"
#include "const.h"
#include "infra/LogManager.h"
#include "repository/FileRepository.h"
#include <cstdint>
#include <filesystem>
#include <grpcpp/support/status.h>

namespace fs = std::filesystem;

QueryDownloadCallData::QueryDownloadCallData(
    FileService::FileTransport::AsyncService* service,
    grpc::ServerCompletionQueue*              cq)
    : _service(service), _cq(cq), _responder(&_ctx), _state(CallState::CREATE) {
    Proceed(true);
}

void QueryDownloadCallData::Proceed(bool ok) {
    switch (_state) {
    case CallState::CREATE: {
        _state = CallState::PROCESS;
        _service->RequestQueryDownloadStatus(
            &_ctx, &_request, &_responder, _cq, _cq, this);
        break;
    }

    case CallState::PROCESS: {
        new QueryDownloadCallData(_service, _cq);

        const std::string file_name  = _request.file_name();
        const std::string session_id = _request.session_id();

        LOG_INFO(
            "[QueryDownloadCallData] Querying download status for file: {}, "
            "session: {}",
            file_name,
            session_id);

        // 查询文件路径
        std::string found_path
            = FileIndexManager::getInstance()->find_path(file_name);

        if (found_path.empty()) {
            // 文件不存在
            _response.set_has_breakpoint(false);
            _response.set_resume_offset(0);
            _response.set_file_size(0);
            LOG_WARN("[QueryDownloadCallData] File not found: {}", file_name);
        } else {
            // 获取文件大小
            int64_t file_size = 0;
            try {
                file_size = fs::file_size(found_path);
            } catch (const std::exception& e) {
                LOG_ERROR(
                    "[QueryDownloadCallData] Failed to get file size: {}",
                    e.what());
                _responder.Finish(
                    _response,
                    grpc::Status(
                        grpc::StatusCode::INTERNAL, "Failed to get file size"),
                    this);
                return;
            }

            // 查询下载断点
            auto progress_result
                = FileRepository::GetDownloadProgress(file_name, session_id);

            if (!progress_result.IsOK()) {
                // 没有断点记录
                _response.set_has_breakpoint(false);
                _response.set_resume_offset(0);
                _response.set_file_size(file_size);
                LOG_INFO(
                    "[QueryDownloadCallData] No breakpoint found for file: {}",
                    file_name);
            } else {
                auto progress = progress_result.Value();
                _response.set_has_breakpoint(true);
                _response.set_resume_offset(progress.downloaded_bytes);
                _response.set_file_size(file_size);

                LOG_INFO(
                    "[QueryDownloadCallData] Breakpoint found for file: {}, "
                    "offset: {}, size: {}",
                    file_name,
                    progress.downloaded_bytes,
                    file_size);
            }
        }

        _state = CallState::FINISH;
        _responder.Finish(_response, grpc::Status::OK, this);
        break;
    }

    case CallState::FINISH: {
        delete this;
        break;
    }
    }
}
