#include "FileWorker.h"
#include "BackPressureManager.h"
#include "FileIndexManager.h"
#include "TaskQueue.h"
#include "infra/LogManager.h"
#include <fstream>
#include <unordered_map>

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
            // create file folder
            std::string filepath
                = "./uploads/" + task.file_md5 + "_" + task.file_name;
            FileBuffer file_buf;
            file_buf.file
                = std::make_unique<std::ofstream>(filepath, std::ios::binary);
            if (file_buf.file->is_open()) {
                _file_buffers[task.file_md5] = std::move(file_buf);
                _md5_to_filename_map[task.file_md5] = task.file_name;
                LOG_INFO("[FileWorker] id: {} File opened {}", _id, filepath);
            } else {
                LOG_INFO(
                    "[FileWorker] id: {} Failed to open file {}",
                    _id,
                    filepath);
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
                    BackPressureManager::getInstance()->notify_data_flushed(
                        flushed_size);
                }
            } else {
                LOG_INFO(
                    "[FileWorker] id: {} Error: Received data for unknown file "
                    "MD5:  {}",
                    _id,
                    task.file_md5);
            }
            break;
        }
        case TaskType::END: {
            auto it = _file_buffers.find(task.file_md5);
            if (it != _file_buffers.end()) {
                size_t remaining_size = it->second.buffer.size();
                it->second.flush();
                BackPressureManager::getInstance()->notify_data_flushed(
                    remaining_size);
                it->second.file->close();
                LOG_INFO(
                    "[FileWorker] id: {}, File closed {}", _id, task.file_md5);
                _file_buffers.erase(it);


                auto map_it = _md5_to_filename_map.find(task.file_md5);
                if(map_it != _md5_to_filename_map.end()) {
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
