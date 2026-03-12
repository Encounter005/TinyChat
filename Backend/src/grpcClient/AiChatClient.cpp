#include "AiChatClient.h"
#include "ai.grpc.pb.h"
#include "infra/ChannelPool.h"
#include "infra/ConfigManager.h"
#include "infra/StubFactory.h"
#include <grpcpp/client_context.h>


AiChatRsp AiChatClient::Chat(
    int uid, const std::string& query,
               const std::vector<std::pair<std::string, std::string&>>& history) {
    AiChatReq req;
    req.set_uid(uid);
    req.set_query(query);
    for(const auto& [role, content] : history) {
        auto* turn = req.add_history();
        turn->set_role(role);
        turn->set_content(content);
    }

    AiChatRsp rsp;
    grpc::ClientContext ctx;
    StubFactory<ai::AiService> stubFactory(_pool);
    auto stub = stubFactory.create();
    grpc::Status st = stub->Chat(&ctx, req, &rsp);
    if(!st.ok()) {
        rsp.set_error(1002); // RPC FAILED
        rsp.set_error_msg(st.error_message());
    }

    return rsp;
}


AiChatClient::AiChatClient() {
    auto cfg = ConfigManager::getInstance();
    const auto host = (*cfg)["AiServer"]["host"];
    const auto port = (*cfg)["AiServer"]["port"];
    _pool = std::make_shared<ChannelPool>(host + ":" + port, 2);
}
