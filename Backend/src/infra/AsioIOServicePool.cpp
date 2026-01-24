#include "AsioIOServicePool.h"
#include "LogManager.h"
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <memory>

AsioIOServicePool::AsioIOServicePool(std::size_t size)
    : _io_services(size), _works(size), _nextIOService(0) {
    for (std::size_t i = 0; i < size; ++i) {
        _works[i] = std::make_unique<WorkGuard>(
            boost::asio::make_work_guard(_io_services[i]));
    }

    for (std::size_t i = 0; i < _io_services.size(); ++i) {
        _threads.emplace_back([this, i]() { _io_services[i].run(); });
    }
    LOG_INFO("AsioIOServicePool Created");
}

AsioIOServicePool::~AsioIOServicePool() {
    Stop();
    LOG_INFO("AsioIOServicePool destructed");
}


boost::asio::io_context& AsioIOServicePool::GetIOService() {
    auto index = _nextIOService ++ % _io_services.size();
    return _io_services[index];
}

void AsioIOServicePool::Stop() {
    for (auto& work : _works) {
        work->reset();
    }

    for (auto& ioc : _io_services) {
        ioc.stop();
    }

    for (auto& t : _threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}
