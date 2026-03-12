#include "AiServiceImpl.h"
#include <grpcpp/support/status.h>

AiServiceImpl::AiServiceImpl(const VaneClient::Config &cfg) : _vane(cfg) {
    
}

grpc::Status AiServiceImpl::Chat(
    grpc::ServerContext *ctx, const ai::AiChatReq *request,
                                 ai::AiChatRsp *reply) {

    std::vector<std::pair<std::string, std::string>> history;
    for(const auto& h : request->history()) {
        history.emplace_back(h.role(), h.content());
    }

    std::string answer;
    std::string err;

    const bool ok = _vane.Search(request->query(), history, answer, err);
    if(!ok) {
        reply->set_error(1002); // RPC FAILED
        reply->set_answer("AI服务暂时不可用，请稍后重试");
        reply->set_error_msg(err);
        return grpc::Status::OK;
    }

    reply->set_error(0);
    reply->set_answer(answer);
    return grpc::Status::OK;
}
