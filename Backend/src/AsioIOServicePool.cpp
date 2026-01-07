#include "AsioIOServicePool.h"
#include "LogManager.h"
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <memory>

AsioIOServicePool::AsioIOServicePool(std::size_t size)
    : _ioService(size), _works(size), _nextIOService(0) {
    LOG_INFO("AsioIOServicePool Created");
    for (std::size_t i = 0; i < size; ++i) {
        _works[i] = std::make_unique<WorkGuard>(
            boost::asio::make_work_guard(_ioService[i]));
    }

    for (std::size_t i = 0; i < _ioService.size(); ++i) {
        _threads.emplace_back([this, i]() { _ioService[i].run(); });
    }
}

AsioIOServicePool::~AsioIOServicePool() {
    Stop();
    LOG_INFO("AsioIOServicePool destructed");
}


boost::asio::io_context& AsioIOServicePool::GetIOService() {
    auto& service = _ioService[_nextIOService++];
    _nextIOService %= _ioService.size();
    return service;
}

void AsioIOServicePool::Stop() {
    for (auto& work : _works) {
        work->reset();
    }

    for (auto& ioc : _ioService) {
        ioc.stop();
    }

    for (auto& t : _threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}
