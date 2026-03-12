#ifndef AISERVICEIMPL_H_
#define AISERVICEIMPL_H_

#include "VaneClient.h"
#include "ai.grpc.pb.h"
#include "ai.pb.h"
#include <grpcpp/server_context.h>
class AiServiceImpl : public ai::AiService::Service {
public:
    explicit AiServiceImpl(const VaneClient::Config& cfg);
    grpc::Status Chat(grpc::ServerContext *ctx, const ai::AiChatReq *request, ai::AiChatRsp *reply) override;
private:
    VaneClient _vane;
};


#endif // AISERVICEIMPL_H_
