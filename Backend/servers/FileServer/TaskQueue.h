#ifndef TASKQUEUE_H_
#define TASKQUEUE_H_
#include "const.h"
#include <mutex>
#include <random>

class FileWorker;

struct FsTask {
    TaskType                     type;
    std::string                  file_name;
    std::string                  file_md5;
    std::shared_ptr<std::string> data;



    static FsTask createMeta(const std::string& name, const std::string& md5) {
        return {TaskType::META, name, md5, nullptr};
    }
    static FsTask createData(const std::string& md5, const std::string& chunk) {
        return {TaskType::DATA, "", md5, std::make_shared<std::string>(chunk)};
    }

    static FsTask createData(const std::string& md5, const std::string&& chunk) {
        return {TaskType::DATA, "", md5, std::make_shared<std::string>(std::move(chunk))};
    }
    static FsTask createEnd(const std::string& md5) {
        return {TaskType::END, "", md5, nullptr};
    }
};

template<typename T> class TaskQueue {
public:
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _queue.push(value);
        }
        _cond.notify_one();
    }

    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock, [this]() { return !_queue.empty() || _stop; });

        if (_stop && _queue.empty()) return false;
        value = std::move(_queue.front());
        _queue.pop();
        return true;
    }

    void stop() {
        _stop = true;
        _cond.notify_all();
    }

private:
    std::queue<T>           _queue;
    std::mutex              _mutex;
    std::condition_variable _cond;
    std::atomic<bool>       _stop{false};
};


#endif   // TASKQUEUE_H_
