#include "FileServer.h"
#include "FileWorker.h"
#include "infra/ConfigManager.h"
#include "const.h"
#include "FileTransportServiceImpl.h"
#include "infra/LogManager.h"
#include <iostream>

auto main(int argc, char *argv[]) -> int {
    system("mkdir -p uploads");

    auto& config = *ConfigManager::getInstance();
    // std::string addr = config["FileServer"]["host"] + ":" +
    //                    config["FileServer"]["port"];


    // // 构建 gRPC 服务
    // FileTransportServiceImpl service;
    // ServerBuilder builder;
    // builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    // builder.RegisterService(&service);
    // builder.SetMaxReceiveMessageSize(1024 * 1024 * 1024);

    // auto server = builder.BuildAndStart();
    // LOG_INFO("[FileServer] listening on {}", addr);

    // server->Wait();
    auto host = config["FileServer"]["host"];
    auto port = config["FileServer"]["port"];
    FileServer::getInstance()->Run(host, port);
    return 0;
}
