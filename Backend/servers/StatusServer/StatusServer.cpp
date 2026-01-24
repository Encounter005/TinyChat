#include "StatusServiceImpl.h"
#include "infra/ConfigManager.h"
#include "infra/LogManager.h"
#include <boost/asio/io_context.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <grpcpp/security/server_credentials.h>
#include <memory>
#include <thread>
void RunServer() {
    auto        globalConfig   = ConfigManager::getInstance();
    auto        host           = (*globalConfig)["StatusServer"]["host"];
    auto        port           = (*globalConfig)["StatusServer"]["port"];
    std::string server_address = host + ":" + port;

    StatusServiceImpl   service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    LOG_INFO("[StatusServer] Server listening on: {}", server_address);

// gRPC Server 负责 RPC,Asio 负责进程生命周期;
// io_context + signal_set 是用来把"OS信号"转成"可控的 C++ 事件";
// 起线程是因为 server->Wait() 本身是阻塞的.

    boost::asio::io_context ioc;
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&server](const boost::system::error_code& error, int signal_number) {
            if (!error) {
                LOG_INFO("[StatusServer] Shutting down server...");
                server->Shutdown(
                    std::chrono::system_clock::now()
                    + std::chrono::seconds(10));   // 发起关闭
            }
        });


    std::thread([&ioc]() { ioc.run(); }).detach();

    server->Wait();   // 等待完成
    ioc.stop();
}

int main() {
    try {
        RunServer();
    } catch (std::exception& e) {
        LOG_ERROR("[StatusServer] exception occurred: {}", e.what());
        return EXIT_FAILURE;
    }
    return 0;
}
