#ifndef ASIOIOSERVICEPOOL_H_
#define ASIOIOSERVICEPOOL_H_

#include "common/singleton.h"
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
    AsioIOServicePool(std::size_t size = 4);
    std::vector<IOService>   _ioService;
    std::vector<WorkPtr>     _works;
    std::vector<std::thread> _threads;
    std::size_t              _nextIOService;
};


#endif   // ASIOIOSERVICEPOOL_H_
