#include "RPConPool.h"
#include "message.grpc.pb.h"
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <memory>
#include <mutex>

RPConPool::RPConPool(std::size_t poolSize, const std::string &host, const std::string& port)
    : _poolSize(poolSize), _host(host), _port(port) {
    std::cout << "target is: " << host + ":" + port << std::endl;
    for (size_t i = 0; i < _poolSize; ++i) {
        std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
            host + ":" + port, grpc::InsecureChannelCredentials());

        _connections.push(message::VarifyService::NewStub(channel));
    }
}


RPConPool::~RPConPool() {
    std::lock_guard<std::mutex> lock(_mutex);
    Close();
    while (!_connections.empty()) {
        _connections.pop();
    }
}

void RPConPool::returnConnection(
    std::unique_ptr<message::VarifyService::Stub> context) {
    std::lock_guard<std::mutex> lock(_mutex);
    if(_stop) {
        return;
    }

    _connections.push(std::move(context));
    _cond.notify_one();
}


std::unique_ptr<message::VarifyService::Stub> RPConPool::getConnection() {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [this]() {
        if (_stop) {
            return true;
        }
        return !_connections.empty();
    });

    if (_stop) {
        return nullptr;
    }

    auto context = std::move(_connections.front());
    _connections.pop();
    return context;
}

void RPConPool::Close() {
    _stop.store(true);
    _cond.notify_all();
}
