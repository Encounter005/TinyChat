#ifndef FILEWORKER_H_
#define FILEWORKER_H_
#include "const.h"
#include "TaskQueue.h"


class FileWorker {
public:
        FileWorker(int id);
        ~FileWorker();
        void submit(const FsTask& task);

private:
        void run();
private:
        int _id;
        std::atomic<bool> _running;
        std::thread _worker_thread;
        TaskQueue<FsTask> _queue;
};


#endif   // FILEWORKER_H_
