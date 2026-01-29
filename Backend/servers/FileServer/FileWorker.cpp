#include "FileWorker.h"
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
    // 维护当前线程打开的文件句柄 map<md5, ofstream>
    std::unordered_map<std::string, std::unique_ptr<std::ofstream>>
        active_files;
    while (_running) {
        FsTask task;
        if (!_queue.pop(task)) {
            break;
        }

        switch (task.type) {
        case TaskType::META: {
            // create file folder
            std::string filepath
                = "./uploads/" + task.file_md5 + "_" + task.file_name;
            auto file
                = std::make_unique<std::ofstream>(filepath, std::ios::binary);
            if (file->is_open()) {
                active_files[task.file_md5] = std::move(file);
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
            auto it = active_files.find(task.file_md5);
            if (it != active_files.end() && it->second->is_open()) {
                it->second->write(task.data.data(), task.data.size());
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
            auto it = active_files.find(task.file_md5);
            if (it != active_files.end()) {
                it->second->close();
                LOG_INFO(
                    "[FileWorker] id: {}, File closed {}", _id, task.file_md5);
                active_files.erase(it);
            }
            break;
        }
        }
    }
}
