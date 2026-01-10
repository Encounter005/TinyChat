#ifndef RPCONPOOL_H_
#define RPCONPOOL_H_

#include "message.grpc.pb.h"
#include "message.pb.h"
#include <atomic>
#include <condition_variable>
#include <grpc++/grpc++.h>
#include <grpcpp/channel.h>
#include <memory>
#include <mutex>
#include <queue>


class RPConPool {
public:
    RPConPool(std::size_t, const std::string&, const std::string&);
    ~RPConPool();
    std::unique_ptr<message::VarifyService::Stub> getConnection();
    void returnConnection(std::unique_ptr<message::VarifyService::Stub>);
    void Close();

private:
    std::atomic<bool>                                         _stop;
    std::size_t                                               _poolSize;
    std::string                                               _host;
    std::string                                               _port;
    std::queue<std::unique_ptr<message::VarifyService::Stub>> _connections;
    std::mutex                                                _mutex;
    std::condition_variable                                   _cond;
};


#endif   // RPCONPOOL_H_
