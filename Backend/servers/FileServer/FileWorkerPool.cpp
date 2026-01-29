#include "FileWorkerPool.h"
#include "FileWorker.h"
#include "infra/LogManager.h"

FileWorkerPool::FileWorkerPool(int count) {
    for (std::size_t i = 0; i < count; ++i) {
        _workers.emplace_back(std::make_unique<FileWorker>(i));
    }
    LOG_INFO("[FileWorkerPool] Init successfully");
}

void FileWorkerPool::Stop() {
    _workers.clear();
}


std::size_t FileWorkerPool::GetWorkerIndex(const std::string& md5) {
    return std::hash<std::string>{}(md5) % _workers.size();
}

void FileWorkerPool::Submit(const FsTask& task) {
    auto idx = GetWorkerIndex(task.file_md5);
    _workers[idx]->submit(task);

}

FileWorkerPool::~FileWorkerPool() {
    Stop();
}
