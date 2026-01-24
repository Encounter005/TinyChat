#include "infra/ChannelPool.h"
#include "infra/ConfigManager.h"
#include "common/const.h"
#include "infra/StubFactory.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include "common/singleton.h"
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>
#include <memory>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;
using message::ErrorCode;


class VarifyGrpcClient : public SingleTon<VarifyGrpcClient> {
public:
    friend class SingleTon<VarifyGrpcClient>;

    GetVarifyRsp GetVarifyCode(const std::string& email) {
        ClientContext context;
        GetVarifyReq  request;
        GetVarifyRsp  reply;
        request.set_email(email);
        auto stub = _stubFactory->create();
        Status status = stub->GetVarifyCode(&context, request, &reply);
        if (status.ok()) {
            return reply;
        } else {
            reply.set_error(ErrorCode::RPC_FAILED);
            return reply;
        }
    }


private:
    VarifyGrpcClient() {
        auto globalConfig = ConfigManager::getInstance();
        auto host         = (*globalConfig)["VarifyServer"]["host"];
        auto port         = (*globalConfig)["VarifyServer"]["port"];
        std::string server_address = host + ":" + port;
        _pool.reset(new ChannelPool(server_address));
        _stubFactory = std::make_unique<StubFactory<VarifyService>>(_pool);
    }
    std::shared_ptr<ChannelPool> _pool;
    std::unique_ptr<StubFactory<VarifyService>> _stubFactory;
};
