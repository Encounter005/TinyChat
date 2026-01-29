#ifndef FILEWORKERPOOL_H_
#define FILEWORKERPOOL_H_

#include "FileWorker.h"
#include "common/singleton.h"
#include <vector>

class FileWorkerPool : public SingleTon<FileWorkerPool> {
    friend class SingleTon<FileWorkerPool>;

public:
        ~FileWorkerPool();
        void Stop();
        void Submit(const FsTask& task);
private:
        std::size_t GetWorkerIndex(const std::string& md5);
private:
        FileWorkerPool(int count = 4);
        std::vector<std::unique_ptr<FileWorker>> _workers;
};


#endif   // FILEWORKERPOOL_H_
