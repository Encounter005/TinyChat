#include "StatusClient.h"
#include "common/const.h"
#include "infra/ChannelPool.h"
#include "infra/ConfigManager.h"
#include "infra/LogManager.h"
#include "infra/StubFactory.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <cstdlib>
#include <grpcpp/client_context.h>

StatusClient::StatusClient() {
    auto        globalConfig   = ConfigManager::getInstance();
    auto        host           = (*globalConfig)["StatusServer"]["host"];
    auto        port           = (*globalConfig)["StatusServer"]["port"];
    auto channelPoolSize_str = (*globalConfig)["GrpcChannelPool"]["StatusServer"];
    auto channelPoolSize = std::atoi(channelPoolSize_str.c_str());
    std::string server_address = host + ":" + port;
    _pool = std::make_shared<ChannelPool>(server_address, channelPoolSize);
    _stubFactory = std::make_unique<StubFactory<message::StatusService>>(_pool);
}

GetChatServerRsp StatusClient::GetChatServer(int uid) {
    ClientContext    context;
    GetChatServerRsp reply;
    GetChatServerReq request;
    request.set_uid(uid);

    auto stub = _stubFactory->create();
    Status status = stub->GetChatServer(&context, request, &reply);
    if (!status.ok()) {
        reply.set_error(ErrorCode::RPC_FAILED);
    }
    return reply;
}

LoginRsp StatusClient::Login(int uid, const std::string& token) {
    ClientContext context;
    LoginReq request;
    LoginRsp reply;
    request.set_uid(uid);
    request.set_token(token);
    auto stub = _stubFactory->create();
    Status status = stub->Login(&context, request, &reply);
    if( !status.ok()) {
        reply.set_error(ErrorCode::RPC_FAILED);
    }
    return reply;
}
