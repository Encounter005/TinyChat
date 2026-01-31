#ifndef FILEWORKER_H_
#define FILEWORKER_H_
#include "TaskQueue.h"
#include "const.h"
#include <unordered_map>


class FileWorker {
public:
    FileWorker(int id);
    ~FileWorker();
    void submit(const FsTask& task);

private:
    void run();

private:
    struct FileBuffer {
        std::unique_ptr<std::ofstream> file;
        std::string                    buffer;
        std::size_t                    total_written = 0;
        void                           flush() {
            if (!buffer.empty() && file->is_open()) {
                file->write(buffer.data(), buffer.size());
                buffer.clear();
            }
        }
    };
    int                                         _id;
    std::atomic<bool>                           _running;
    std::thread                                 _worker_thread;
    TaskQueue<FsTask>                           _queue;
    std::unordered_map<std::string, FileBuffer> _file_buffers;
    std::unordered_map<std::string, std::string> _md5_to_filename_map;
    static constexpr size_t                     BUFFER_THRESHOLD = 256 * 1024;
};


#endif   // FILEWORKER_H_
