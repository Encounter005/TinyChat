#include "FileWorker.h"
#include "BackPressureManager.h"
#include "FileIndexManager.h"
#include "repository/FileRepository.h"
#include "TaskQueue.h"
#include "infra/LogManager.h"
#include <fstream>
#include <unordered_map>
#include <openssl/md5.h>

FileWorker::FileWorker(int id) : _id(id), _running(true) {
    _worker_thread = std::thread(&FileWorker::run, this);
}

FileWorker::~FileWorker() {
    _running = false;
    _queue.stop();
    if (_worker_thread.joinable()) {
        _worker_thread.join();
    }
}

void FileWorker::submit(const FsTask& task) {
    _queue.push(task);
}

// 新增：MD5计算函数
std::string FileWorker::CalculateMD5(const std::string& data) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5((unsigned char*)data.data(), data.size(), digest);

    char md5_string[33];
    for (int i = 0; i < 16; i++) {
        sprintf(&md5_string[i * 2], "%02x", (unsigned int)digest[i]);
    }

    return std::string(md5_string, 32);
}

void FileWorker::run() {
    LOG_INFO("[FileWorker] id: {} started", _id);
    while (_running) {
        FsTask task;
        if (!_queue.pop(task)) {
            if (!_running) break;
            continue;
        }

        switch (task.type) {
        case TaskType::META: {
            // create file
            std::string filepath = "./uploads/" + task.file_md5 + "_" + task.file_name;

            std::ios::openmode mode = std::ios::binary;
            if (task.resume_offset > 0) {
                mode |= std::ios::app;  // 追加模式
                LOG_INFO("[FileWorker] id: {} Opening file for resume: {} at offset {}",
                         _id, filepath, task.resume_offset);
            } else {
                mode |= std::ios::trunc;  // 覆盖模式
                LOG_INFO("[FileWorker] id: {} Creating new file: {}", _id, filepath);
            }

            FileBuffer file_buf;
            file_buf.file = std::make_unique<std::ofstream>(filepath, mode);
            file_buf.file_md5 = task.file_md5;
            file_buf.file_name = task.file_name;

            if (file_buf.file->is_open()) {
                // 验证文件大小
                if (task.resume_offset > 0) {
                    file_buf.file->seekp(0, std::ios::end);
                    auto current_size = static_cast<int64> ( file_buf.file->tellp() );
                    if (current_size != task.resume_offset) {
                        LOG_WARN(
                            "[FileWorker] id: {} File size mismatch: expected={}, actual={}",
                            _id, task.resume_offset, current_size);
                    } else {
                        file_buf.total_written = task.resume_offset;
                    }
                }

                _file_buffers[task.file_md5] = std::move(file_buf);
                _md5_to_filename_map[task.file_md5] = task.file_name;
                LOG_INFO("[FileWorker] id: {} File opened {}", _id, filepath);
            } else {
                LOG_ERROR("[FileWorker] id: {} Failed to open file {}", _id, filepath);
            }
            break;
        }

        case TaskType::DATA: {
            auto it = _file_buffers.find(task.file_md5);
            if (it != _file_buffers.end() && task.data) {
                auto& file_buf = it->second;
                file_buf.buffer.append(*(task.data));

                // 超过阈值就刷新缓冲区
                if (file_buf.buffer.size() >= BUFFER_THRESHOLD) {
                    size_t flushed_size = file_buf.buffer.size();
                    file_buf.flush();
                    BackPressureManager::getInstance()->notify_data_flushed(flushed_size);
                }

                // 更新总写入字节数
                file_buf.total_written += task.data->size();

                // 每20MB保存断点和校验和
                if (file_buf.total_written % RESUME_CHUNK_SIZE == 0) {
                    int64 chunk_index = file_buf.total_written / RESUME_CHUNK_SIZE - 1;

                    // 计算分块MD5
                    std::string chunk_md5 = CalculateMD5(file_buf.buffer);

                    // 保存分块校验和
                    auto result = FileRepository::SaveBlockCheckpoint(
                        file_buf.file_md5,
                        chunk_index,
                        chunk_md5);

                    if (result.IsOK()) {
                        LOG_INFO(
                            "[FileWorker] id: {} Saved chunk checkpoint: md5={}, chunk={}, offset={}",
                            _id, file_buf.file_md5, chunk_index, file_buf.total_written);
                    }

                    // 更新上传进度
                    FileRepository::UpdateUploadProgress(
                        file_buf.file_md5,
                        file_buf.total_written);

                    // 清空缓冲区
                    file_buf.buffer.clear();
                }
            } else {
                LOG_ERROR(
                    "[FileWorker] id: {} Error: Received data for unknown file MD5: {}",
                    _id, task.file_md5);
            }
            break;
        }

        case TaskType::END: {
            auto it = _file_buffers.find(task.file_md5);
            if (it != _file_buffers.end()) {
                size_t remaining_size = it->second.buffer.size();
                it->second.flush();
                BackPressureManager::getInstance()->notify_data_flushed(remaining_size);
                it->second.file->close();

                LOG_INFO("[FileWorker] id: {}, File closed {}", _id, task.file_md5);

                // 标记上传完成
                FileRepository::MarkUploadComplete(task.file_md5);

                // 清除断点记录
                FileRepository::DeleteUploadProgress(task.file_md5);
                FileRepository::DeleteBlockCheckpoints(task.file_md5);

                _file_buffers.erase(it);

                auto map_it = _md5_to_filename_map.find(task.file_md5);
                if (map_it != _md5_to_filename_map.end()) {
                    std::string original_name = map_it->second;
                    std::string full_path = "./uploads/" + task.file_md5 + "_" + original_name;
                    FileIndexManager::getInstance()->add_file(original_name, full_path);
                    _md5_to_filename_map.erase(map_it);
                }
            }
            break;
        }
        }
    }
}
