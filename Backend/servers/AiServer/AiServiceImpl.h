#ifndef AISERVICEIMPL_H_
#define AISERVICEIMPL_H_

#include "AstrBotClient.h"
#include "VaneClient.h"
#include "ai.grpc.pb.h"
#include "ai.pb.h"
#include <grpcpp/server_context.h>
class AiServiceImpl : public ai::AiService::Service {
public:
    AiServiceImpl(
        const VaneClient::Config    &vane_cfg,
        const AstrBotClient::Config &astrbot_cfg);
    grpc::Status Chat(
        grpc::ServerContext *ctx, const ai::AiChatReq *request,
        ai::AiChatRsp *reply) override;

private:
    AstrBotClient _astrbot;
    VaneClient    _vane;
};


#endif   // AISERVICEIMPL_H_
