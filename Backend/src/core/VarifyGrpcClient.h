#include "infra/ConfigManager.h"
#include "infra/RPConPool.h"
#include "common/const.h"
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


class VarifyGrpcClient : public SingleTon<VarifyGrpcClient> {
public:
    friend class SingleTon<VarifyGrpcClient>;

    GetVarifyRsp GetVarifyCode(const std::string& email) {
        ClientContext context;
        GetVarifyReq  request;
        GetVarifyRsp  reply;
        request.set_email(email);

        auto   stub   = _rpcPool->getConnection();
        Status status = stub->GetVarifyCode(&context, request, &reply);
        if (status.ok()) {
            return reply;
        } else {
            reply.set_error(ErrorCodes::RPC_FAILED);
            return reply;
        }
    }


private:
    VarifyGrpcClient() {
        auto globalConfig = ConfigManager::getInstance();
        auto host         = (*globalConfig)["VarifyServer"]["host"];
        auto port         = (*globalConfig)["VarifyServer"]["port"];
        _rpcPool.reset(new RPConPool(5, host, port));
    }
    std::unique_ptr<VarifyService::Stub> _stub;
    std::unique_ptr<RPConPool>           _rpcPool;
};
