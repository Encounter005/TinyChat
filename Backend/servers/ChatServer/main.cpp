#include "ChatServer.h"
#include "ChatServiceImpl.h"
#include "common/ChatServerInfo.h"
#include "infra/AsioIOServicePool.h"
#include "infra/ConfigManager.h"
#include "infra/LogManager.h"
#include "infra/RedisManager.h"
#include "repository/ChatServerRepository.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <cstdlib>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>
#include <iostream>
#include <string>

std::unique_ptr<grpc::Server> StartRPCServer(
    const std::string& server_address) {
    static ChatServiceImpl service;
    grpc::ServerBuilder    builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server) {
        LOG_ERROR("Failed to start RPC server at {}", server_address);
        return nullptr;
    }
    LOG_INFO("RPC server listening on: {}", server_address);
    return server;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Useage: chatserver <ServerName>\n";
        return 1;
    }
    try {
        std::string    ServerName   = argv[1];
        auto           globalConfig = ConfigManager::getInstance();
        auto           port_str     = (*globalConfig)[ServerName]["port"];
        auto           rpc_port     = (*globalConfig)[ServerName]["RPCport"];
        unsigned short port
            = static_cast<unsigned short>(atoi(port_str.c_str()));

        ChatServerRepository::RestConnection(ServerName);

        auto rpc_server = StartRPCServer(
            (*globalConfig)[ServerName]["host"] + ":" + rpc_port);
        std::thread grpc_thread([&rpc_server]() { rpc_server->Wait(); });

        boost::asio::io_context ioc;

        ChatServerInfo server_info;
        server_info.host = (*globalConfig)[ServerName]["host"];
        server_info.port = port_str;
        server_info.name = ServerName;

        // 必须保存 server 对象，不要写成临时对象
        ChatServer server(ioc, port, server_info);

        // 信号处理
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc, &rpc_server, ServerName](auto, auto) {
            LOG_INFO("Shutting down servers....");
            if (rpc_server) {
                rpc_server->Shutdown();
            }
            ioc.stop();
            ChatServerRepository::RestConnection(ServerName);
            AsioIOServicePool::getInstance()->Stop();
        });

        LOG_INFO("[ChatServer] running...");
        ioc.run();

        grpc_thread.join();


    } catch (const std::exception& e) {
        LOG_ERROR("[ChatServer] exception: {}", e.what());
    }
    return 0;
}
