#ifndef FILESERVER_H_
#define FILESERVER_H_

#include "common/singleton.h"
#include "const.h"
#include "file.grpc.pb.h"
#include <grpcpp/completion_queue.h>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
class FileServer : public SingleTon<FileServer> {
    friend class SingleTon<FileServer>;

public:
    void Run(const std::string& host, const std::string& port);
    void Stop();
    ~FileServer();
private:
    FileServer();

    void SetupServer();
    void StartEventLoop();
    void ProceedCalls();

private:
    std::string _host;
    std::string _port;
    std::unique_ptr<Server> _server;
    std::unique_ptr<ServerCompletionQueue> _cq;
    std::unique_ptr<FileTransport::AsyncService> _service;
    std::thread _event_loop;
    std::unordered_map<std::string, UploadSession> _sessions;
    std::mutex _sessions_mtx;
};


#endif   // FILESERVER_H_
