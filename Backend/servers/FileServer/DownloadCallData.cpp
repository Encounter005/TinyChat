#include "DownloadCallData.h"
#include "FileIndexManager.h"
#include "repository/FileRepository.h"
#include "const.h"
#include "file.pb.h"
#include "infra/LogManager.h"
#include <filesystem>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/status.h>
#include <ios>

namespace fs = std::filesystem;

std::string DownloadCallData::GenerateSessionID() {
    boost::uuids::random_generator gen;
    boost::uuids::uuid uuid = gen();
    return boost::uuids::to_string(uuid);
}

DownloadCallData::DownloadCallData(
    FileTransport::AsyncService* service, ServerCompletionQueue* cq)
    : _service(service)
    , _cq(cq)
    , _writer(&_context)
    , _state(CallState::CREATE)
    , _buffer(2 * 1024 * 1024) {
    Proceed(true);
}

void DownloadCallData::Proceed(bool ok) {
    switch (_state) {
    case CallState::CREATE: {
        _state = CallState::PROCESS;
        _service->RequestDownloadFile(
            &_context, &_request, &_writer, _cq, _cq, this);
        break;
    }

    case CallState::PROCESS: {
        new DownloadCallData(_service, _cq);

        std::string request_filename = _request.file_name();
        _start_offset = _request.start_offset();   // 获取起始偏移量

        LOG_INFO(
            "[DownloadCallData] Download request: file={}, offset={}",
            request_filename,
            _start_offset);

        // 查找文件路径
        std::string found_path = FileIndexManager::getInstance()->find_path(request_filename);

        if (found_path.empty()) {
            LOG_WARN(
                "[DownloadCallData] File not found for request: {}",
                request_filename);
            _state = CallState::FINISH;
            _writer.Finish(
                Status(grpc::StatusCode::NOT_FOUND, "file not found"), this);
            return;
        }

        // 获取文件大小
        try {
            _file_size = fs::file_size(found_path);
        } catch (std::exception& e) {
            LOG_ERROR("[DownloadCallData] exception occurred: {}", e.what());
            _state = CallState::FINISH;
            _writer.Finish(
                Status(grpc::StatusCode::INTERNAL, "Failed to get file size"), this);
            return;
        }

        // 验证offset
        if (_start_offset < 0 || _start_offset > _file_size) {
            LOG_WARN(
                "[DownloadCallData] Invalid offset: {}, file size: {}",
                _start_offset, _file_size);
            _state = CallState::FINISH;
            _writer.Finish(
                Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid start_offset"),
                this);
            return;
        }

        LOG_INFO(
            "[DownloadCallData] Found file {} for request {}, size={}",
            found_path, request_filename, _file_size);

        _file_stream.open(found_path, std::ios::binary);

        if (!_file_stream.is_open()) {
            _state = CallState::FINISH;
            _writer.Finish(
                grpc::Status(
                    grpc::StatusCode::INTERNAL,
                    "Failed to open file on server"),
                this);
            return;
        }

        // 定位到指定偏移量
        if (_start_offset > 0) {
            _file_stream.seekg(_start_offset);
            _current_offset = _start_offset;
            LOG_INFO("[DownloadCallData] Seeking to offset: {}", _start_offset);
        } else {
            _current_offset = 0;
        }

        // 生成会话ID
        _session_id = GenerateSessionID();
        LOG_INFO("[DownloadCallData] Generated session ID: {}", _session_id);

        // 保存初始断点
        FileRepository::SaveDownloadProgress(
            request_filename,
            _session_id,
            _current_offset);

        // 发送文件元数据
        DownloadResponse meta_response;
        FileMeta* meta = meta_response.mutable_meta();
        meta->set_file_name(request_filename);
        meta->set_total_size(_file_size);

        _state = CallState::STREAM;
        _writer.Write(meta_response, this);
        break;
    }

    case CallState::FINISH: {
        if (_file_stream.is_open()) {
            _file_stream.close();
        }

        // 清除断点记录
        if (!_session_id.empty() && !_request.file_name().empty()) {
            FileRepository::DeleteDownloadProgress(
                _request.file_name(),
                _session_id);
        }

        delete this;
        break;
    }

    case CallState::STREAM: {
        if (!ok) {
            // 客户端取消或断开
            LOG_INFO(
                "[DownloadCallData] Client cancelled or disconnected, saving breakpoint");

            // 保存当前断点
            if (!_session_id.empty() && !_request.file_name().empty()) {
                FileRepository::UpdateDownloadProgress(
                    _request.file_name(),
                    _session_id,
                    _current_offset);
            }

            _state = CallState::FINISH;
            _writer.Finish(Status::CANCELLED, this);
            return;
        }

        DownloadResponse chunk_response;
        std::string* chunk_data = chunk_response.mutable_chunk();
        const size_t chunk_size = 2 * 1024 * 1024;  // 2MB
        chunk_data->resize(chunk_size);

        _file_stream.read(chunk_data->data(), chunk_size);
        std::streamsize bytes_read = _file_stream.gcount();

        if (bytes_read > 0) {
            chunk_data->resize(bytes_read);
            _current_offset += bytes_read;

            // 发送数据块
            _writer.Write(chunk_response, this);

            // 每20MB保存一次断点
            if (_current_offset % SAVE_BREAKPOINT_INTERVAL == 0) {
                LOG_INFO(
                    "[DownloadCallData] Saving breakpoint at offset: {}",
                    _current_offset);
                FileRepository::UpdateDownloadProgress(
                    _request.file_name(),
                    _session_id,
                    _current_offset);
            }
        } else {
            // 文件传输完成
            LOG_INFO(
                "[DownloadCallData] File streaming finished for request: {}, total bytes: {}",
                _request.file_name(), _current_offset);

            // 删除断点记录
            FileRepository::DeleteDownloadProgress(
                _request.file_name(),
                _session_id);

            _state = CallState::FINISH;
            _writer.Finish(Status::OK, this);
        }

        break;
    }
    }
}
