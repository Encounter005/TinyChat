#include "DownloadCallData.h"
#include "FileIndexManager.h"
#include "const.h"
#include "file.pb.h"
#include "infra/LogManager.h"
#include "repository/FileRepository.h"
#include <chrono>
#include <filesystem>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/status.h>
#include <ios>
#include <ratio>

namespace fs = std::filesystem;

std::string DownloadCallData::GenerateSessionID() {
    boost::uuids::random_generator gen;
    boost::uuids::uuid             uuid = gen();
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
        handleCreateState();
        break;
    }

    case CallState::PROCESS: {
        handleProcessState(ok);
        break;
    }

    case CallState::FINISH: {
        handleFinishState();
        break;
    }

    case CallState::STREAM: {
        handleStreamState(ok);
        break;
    }
    }
}




void DownloadCallData::handleCreateState() {
    _state = CallState::PROCESS;
    _service->RequestDownloadFile(
        &_context, &_request, &_writer, _cq, _cq, this);
}

void DownloadCallData::handleProcessState(bool ok) {
    if (!ok) {
        finishWithError(grpc::StatusCode::CANCELLED, "Request Cancelled");
        return;
    }

    createNextHandler();

    std::string request_filename = _request.file_name();
    _start_offset                = _request.start_offset();   // 获取起始偏移量

    LOG_INFO(
        "[DownloadCallData] Download request: file={}, offset={}",
        request_filename,
        _start_offset);

    auto validation = validateDownloadRequest(request_filename, _start_offset);
    if (!validation.is_valid) {
        if (validation.error_code == grpc::StatusCode::NOT_FOUND) {
            handleFileNotFoundError(request_filename);
        } else if (
            validation.error_code == grpc::StatusCode::INVALID_ARGUMENT) {
            handleInvalidOffsetError(_start_offset, validation.file_size);
        } else {
            finishWithError(validation.error_code, validation.error_message);
        }
        return;
    }

    if (!initializeDownloadSession(
            request_filename,
            validation.file_path,
            _start_offset,
            validation.file_size)) {
        finishWithError(
            grpc::StatusCode::INTERNAL, "Failed to initialize session");
        return;
    }

    if (!openFileStream(validation.file_path, _start_offset)) {
        handleFileOpenError();
        return;
    }

    // 保存初始断点
    FileRepository::SaveDownloadProgress(
        request_filename, _session_id, _current_offset);

    sendFileMetadata(request_filename, validation.file_size);

    // perforamnce metrics
    _start_time = std::chrono::high_resolution_clock::now();
}

void DownloadCallData::handleStreamState(bool ok) {
    if (!ok) {
        handleDownloadCancelled();
        return;
    }

    // 读取并发送数据块
    readAndSendChunk();
}

void DownloadCallData::handleFinishState() {
    // 关闭文件流
    if (_file_stream.is_open()) {
        _file_stream.close();
    }

    // 清除断点记录
    if (!_session_id.empty() && !_request.file_name().empty()) {
        FileRepository::DeleteDownloadProgress(
            _request.file_name(), _session_id);
    }

    cleanup();
}

DownloadValidationResult DownloadCallData::validateDownloadRequest(
    const std::string& file_name, int64 start_offset) {

    DownloadValidationResult result;
    result.is_valid = false;

    // 查找文件路径
    std::string file_path
        = FileIndexManager::getInstance()->find_path(file_name);
    if (file_path.empty()) {
        result.error_code    = grpc::StatusCode::NOT_FOUND;
        result.error_message = "File not found: " + file_name;
        return result;
    }
    result.file_path = file_path;

    // 获取文件大小
    try {
        result.file_size = fs::file_size(file_path);
    } catch (const std::exception& e) {
        result.error_code = grpc::StatusCode::INTERNAL;
        result.error_message
            = "Failed to get file size: " + std::string(e.what());
        return result;
    }

    // 验证偏移量
    if (start_offset < 0 || start_offset > result.file_size) {
        result.error_code = grpc::StatusCode::INVALID_ARGUMENT;
        result.error_message
            = "Invalid start_offset: " + std::to_string(start_offset);
        return result;
    }

    result.is_valid = true;
    return result;
}


// file operation
bool DownloadCallData::openFileStream(
    const std::string& file_path, int64 start_offset) {

    auto open_start = std::chrono::high_resolution_clock::now();

    _file_stream.open(file_path, std::ios::binary);

    if (!_file_stream.is_open()) {
        return false;
    }

    auto open_end = std::chrono::high_resolution_clock::now();

    auto seek_start = std::chrono::high_resolution_clock::now();

    if (start_offset > 0) {
        _file_stream.seekg(start_offset);
        LOG_INFO("[DownloadCallData] Seeking to offset: {}", start_offset);
    }

    auto seek_end = std::chrono::high_resolution_clock::now();
    _metrics.file_seek_duration_ms
        = std::chrono::duration<double, std::milli>(seek_end - seek_start)
              .count();
    _metrics.file_open_duration_ms
        = std::chrono::duration<double, std::milli>(open_end - open_start)
              .count();

    return true;
}

void DownloadCallData::sendFileMetadata(
    const std::string& file_name, int64 file_size) {

    DownloadResponse meta_response;
    FileMeta*        meta = meta_response.mutable_meta();
    meta->set_file_name(file_name);
    meta->set_total_size(file_size);

    _state = CallState::STREAM;
    _writer.Write(meta_response, this);
}

// stream handle
void DownloadCallData::readAndSendChunk() {
    DownloadResponse chunk_response;
    std::string*     chunk_data = chunk_response.mutable_chunk();
    chunk_data->resize(CHUNK_SIZE);

    // read time
    auto read_start = std::chrono::high_resolution_clock::now();

    _file_stream.read(chunk_data->data(), CHUNK_SIZE);
    std::streamsize bytes_read = _file_stream.gcount();

    auto   read_end = std::chrono::high_resolution_clock::now();
    double read_duration
        = std::chrono::duration<double, std::milli>(read_end - read_start)
              .count();

    _metrics.file_read_duration_ms += read_duration;

    if (bytes_read > 0) {
        chunk_data->resize(bytes_read);
        _current_offset += bytes_read;

        auto write_start = std::chrono::high_resolution_clock::now();
        _writer.Write(chunk_response, this);
        auto write_end = std::chrono::high_resolution_clock::now();

        double write_duration
            = std::chrono::duration<double, std::milli>(write_end - write_start)
                  .count();
        _metrics.network_write_duration_ms += write_duration;

        // update metrics
        _metrics.bytes_sent += bytes_read;
        _metrics.chunk_count++;
    } else {
        LOG_INFO(
            "[DownloadCallData] Transfer complete: offset={}", _current_offset);
        handleDownloadComplete();
    }


    if (shouldSaveBreakPoint()) {
        saveDownloadBreakPoint();
    }
}

bool DownloadCallData::shouldSaveBreakPoint() const {
    return _current_offset % SAVE_BREAKPOINT_INTERVAL == 0;
}

void DownloadCallData::saveDownloadBreakPoint() {
    LOG_INFO(
        "[DownloadCallData] Saving breakpoint at offset: {}", _current_offset);

    FileRepository::UpdateDownloadProgress(
        _request.file_name(), _session_id, _current_offset);
}

void DownloadCallData::handleDownloadComplete() {
    // 计算总耗时
    auto end_time = std::chrono::high_resolution_clock::now();
    _metrics.total_duration_ms
        = std::chrono::duration<double, std::milli>(end_time - _start_time)
              .count();

    // 记录完成日志
    LOG_INFO(
        "[DownloadCallData] File streaming finished: {}, total bytes: {}",
        _request.file_name(),
        _current_offset);

    // 输出性能汇总
    _metrics.log_summary();

    // 状态转换
    _state = CallState::FINISH;
    _writer.Finish(Status::OK, this);
}

void DownloadCallData::handleDownloadCancelled() {
    // 计算总耗时
    auto end_time = std::chrono::high_resolution_clock::now();
    _metrics.total_duration_ms
        = std::chrono::duration<double, std::milli>(end_time - _start_time)
              .count();

    // 保存断点
    if (!_session_id.empty() && !_request.file_name().empty()) {
        FileRepository::UpdateDownloadProgress(
            _request.file_name(), _session_id, _current_offset);
    }

    // 记录取消日志
    LOG_INFO(
        "[DownloadCallData] Client cancelled at offset: {}", _current_offset);

    // 输出性能汇总
    _metrics.log_summary();

    // 状态转换
    _state = CallState::FINISH;
    _writer.Finish(Status::CANCELLED, this);
}


void DownloadCallData::finishWithError(
    grpc::StatusCode code, const std::string& message) {

    LOG_ERROR("[DownloadCallData] Error: {}", message);
    _state = CallState::FINISH;
    _writer.Finish(Status(code, message), this);
}

void DownloadCallData::handleFileNotFoundError(const std::string& file_name) {
    LOG_WARN("[DownloadCallData] File not found: {}", file_name);
    finishWithError(
        grpc::StatusCode::NOT_FOUND, "File not found: " + file_name);
}

void DownloadCallData::handleFileSizeError() {
    LOG_ERROR("[DownloadCallData] Failed to get file size");
    finishWithError(grpc::StatusCode::INTERNAL, "Failed to get file size");
}

void DownloadCallData::handleInvalidOffsetError(int64 offset, int64 file_size) {
    LOG_WARN(
        "[DownloadCallData] Invalid offset: {}, file size: {}",
        offset,
        file_size);
    finishWithError(
        grpc::StatusCode::INVALID_ARGUMENT,
        "Invalid start_offset: " + std::to_string(offset));
}

void DownloadCallData::handleFileOpenError() {
    LOG_ERROR("[DownloadCallData] Failed to open file");
    finishWithError(
        grpc::StatusCode::INTERNAL, "Failed to open file on server");
}

void DownloadCallData::createNextHandler() {
    new DownloadCallData(_service, _cq);
}

void DownloadCallData::cleanup() {
    delete this;
}


bool DownloadCallData::initializeDownloadSession(
    const std::string& filename, const std::string& file_path,
    int64_t start_offset, int64_t file_size) {

    // 直接设置成员变量 (唯一数据源)
    _session_id     = GenerateSessionID();
    _start_offset   = start_offset;
    _current_offset = start_offset;
    _file_size      = file_size;

    // 初始化性能监控
    _metrics            = DownloadPerformanceMetrics{};
    _metrics.session_id = _session_id;
    _metrics.file_name  = filename;

    LOG_INFO(
        "[DownloadCallData] Created session: {} for file: {}, size: {} bytes",
        _session_id,
        filename,
        file_size);

    return true;
}
