#ifndef ASIOIOSERVICEPOOL_H_
#define ASIOIOSERVICEPOOL_H_

#include "common/singleton.h"
#include <atomic>
#include <boost/asio.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>


class AsioIOServicePool : public SingleTon<AsioIOServicePool> {
    friend class SingleTon<AsioIOServicePool>;

public:
    using IOService = boost::asio::io_context;
    using WorkGuard = boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>;
    using WorkPtr = std::unique_ptr<WorkGuard>;
    ~AsioIOServicePool();
    AsioIOServicePool(const AsioIOServicePool&)                  = delete;
    AsioIOServicePool        operator=(const AsioIOServicePool&) = delete;
    void                     Stop();
    boost::asio::io_context& GetIOService();

private:
    AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency());
    std::vector<IOService>   _io_services;
    std::vector<WorkPtr>     _works;
    std::vector<std::thread> _threads;
    std::atomic_size_t       _nextIOService;
};


#endif   // ASIOIOSERVICEPOOL_H_
