#include "QueryUploadCallData.h"
#include "const.h"
#include "infra/LogManager.h"
#include "repository/FileRepository.h"
#include <cstdint>
#include <grpcpp/support/status.h>

QueryUploadCallData::QueryUploadCallData(
    FileService::FileTransport::AsyncService* service,
    grpc::ServerCompletionQueue*              cq)
    : _service(service), _cq(cq), _responder(&_ctx), _state(CallState::CREATE) {
    Proceed(true);
}

void QueryUploadCallData::Proceed(bool ok) {
    switch (_state) {
    case CallState::CREATE: {
        _state = CallState::PROCESS;
        _service->RequestQueryUploadStatus(
            &_ctx, &_request, &_responder, _cq, _cq, this);
        break;
    }

    case CallState::PROCESS: {
        new QueryUploadCallData(_service, _cq);

        const std::string file_md5 = _request.file_md5();

        LOG_INFO(
            "[QueryUploadCallData] Querying upload status for md5: {}",
            file_md5);

        // 查询上传进度
        auto progress_result = FileRepository::GetUploadProgress(file_md5);

        if (!progress_result.IsOK()) {
            // 没有断点记录
            _response.set_has_breakpoint(false);
            _response.set_resume_offset(0);
            LOG_INFO(
                "[QueryUploadCallData] No breakpoint found for md5: {}",
                file_md5);
        } else {
            auto progress = progress_result.Value();

            // 验证分块完整性
            auto verified_offset_result = FileRepository::VerifyUploadBlocks(
                file_md5,
                progress.uploaded_bytes,
                FileRepository::RESUME_BLOCK_SIZE);

            int64_t resume_offset = 0;
            if (verified_offset_result.IsOK()) {
                resume_offset = verified_offset_result.Value();
            }

            // 获取已上传分块列表
            auto chunks_result = FileRepository::GetUploadedChunks(
                file_md5, FileRepository::RESUME_BLOCK_SIZE);

            _response.set_has_breakpoint(true);
            _response.set_resume_offset(resume_offset);
            _response.set_last_update_time(progress.updated_at);

            if (chunks_result.IsOK()) {
                for (const auto& chunk : chunks_result.Value()) {
                    auto* chunk_info = _response.add_uploaded_chunks();
                    chunk_info->set_chunk_index(chunk.chunk_index);
                    chunk_info->set_chunk_md5(chunk.chunk_md5);
                    chunk_info->set_chunk_offset(chunk.chunk_offset);
                }
            }

            LOG_INFO(
                "[QueryUploadCallData] Breakpoint found for md5: {}, "
                "offset: "
                "{}, chunks: {}",
                file_md5,
                resume_offset,
                _response.uploaded_chunks_size());
        }

        _state = CallState::FINISH;
        _responder.Finish(_response, grpc::Status::OK, this);
        LOG_DEBUG("FINISH");
        break;
    }

    case CallState::FINISH: {
        delete this;
        break;
    }
    }
}
