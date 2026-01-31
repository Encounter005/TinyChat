#include "DownloadCallData.h"
#include "const.h"
#include "file.pb.h"
#include "infra/LogManager.h"
#include <filesystem>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/status.h>
#include <ios>

namespace fs = std::filesystem;

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
        std::string found_path;

        try {
            // 在目录中查找文件
            for (const auto& entry : fs::directory_iterator("./uploads")) {
                if (entry.is_regular_file()) {
                    std::string filename_on_disk
                        = entry.path().filename().string();
                    std::string expected_suffix = "_" + request_filename;
                    if (filename_on_disk.size() > expected_suffix.size()
                        && filename_on_disk.substr(
                               filename_on_disk.size() - expected_suffix.size())
                               == expected_suffix) {
                        found_path = entry.path().string();
                        break;
                    }
                }
            }
        } catch (std::exception& e) {
            LOG_ERROR("[DownloadCallData] exception occurred: {}", e.what());
        }
        if (found_path.empty()) {
            LOG_WARN(
                "[DownloadCallData] File not found for request: {}",
                request_filename);
            _state = CallState::FINISH;
            _writer.Finish(
                Status(grpc::StatusCode::NOT_FOUND, "file not found"), this);
        } else {
            LOG_INFO(
                "[DownloadCallData] Found file {} for request {}",
                found_path,
                request_filename);
            _file_stream.open(found_path, std::ios::binary);

            if (!_file_stream.is_open()) {
                _state = CallState::FINISH;
                _writer.Finish(
                    grpc::Status(
                        grpc::StatusCode::INTERNAL,
                        "Failed to open file on server"),
                    this);
            } else {
                DownloadResponse meta_response;
                FileMeta*        meta = meta_response.mutable_meta();
                meta->set_file_name(request_filename);
                meta->set_total_size(fs::file_size(found_path));
                _state = CallState::STREAM;
                _writer.Write(meta_response, this);
            }
        }

        break;
    }
    case CallState::FINISH: {
        if (_file_stream.is_open()) {
            _file_stream.close();
        }
        delete this;
        break;
    }
    case CallState::STREAM: {
        if (!ok) {   // 客户端取消
            _state = CallState::FINISH;
            _writer.Finish(Status::CANCELLED, this);
        }

        DownloadResponse chunk_response;
        std::string*     chunk_data = chunk_response.mutable_chunk();
        const size_t     chunk_size = 2 * 1024 * 1024;
        chunk_data->resize(chunk_size);

        _file_stream.read(chunk_data->data(), chunk_size);
        std::streamsize bytes_read = _file_stream.gcount();

        if (bytes_read > 0) {
            chunk_data->resize(bytes_read);
            _writer.Write(chunk_response, this);
        } else {
            // 没有就说明已经读到文件尾
            LOG_INFO(
                "[DownloadCallData] File streaming finished for request: {}",
                _request.file_name());
            _state = CallState::FINISH;
            _writer.Finish(Status::OK, this);
        }


        break;
    }
    }
}
