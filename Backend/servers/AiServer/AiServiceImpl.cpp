#include "AiServiceImpl.h"

#include "infra/LogManager.h"

#include <algorithm>
#include <cctype>
#include <grpcpp/support/status.h>

namespace {
std::string NormalizePlatform(std::string platform) {
    std::transform(
        platform.begin(),
        platform.end(),
        platform.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (platform.empty()) {
        return "astrbot";
    }
    return platform;
}
}   // namespace

AiServiceImpl::AiServiceImpl(
    const VaneClient::Config&    vane_cfg,
    const AstrBotClient::Config& astrbot_cfg)
    : _astrbot(astrbot_cfg), _vane(vane_cfg) {}

grpc::Status AiServiceImpl::Chat(
    grpc::ServerContext* ctx, const ai::AiChatReq* request,
    ai::AiChatRsp* reply) {

    std::vector<std::pair<std::string, std::string>> history;
    for (const auto& h : request->history()) {
        history.emplace_back(h.role(), h.content());
    }

    std::string answer;
    std::string err;

    const std::string platform = NormalizePlatform(request->platform());
    LOG_INFO(
        "[AiServiceImpl] chat uid={} platform={} query={} history_size={}",
        request->uid(),
        platform,
        request->query(),
        history.size());
    const bool        ok
        = platform == "vane"
              ? _vane.Search(request->query(), history, answer, err)
              : _astrbot.Chat(
                    request->uid(), request->query(), platform, answer, err);
    if (!ok) {
        LOG_ERROR("[AiServiceImpl] upstream failed platform={} error={}", platform, err);
        reply->set_error(1002);   // RPC FAILED
        reply->set_answer("AI服务暂时不可用，请稍后重试");
        reply->set_error_msg(err);
        return grpc::Status::OK;
    }

    LOG_INFO("[AiServiceImpl] upstream ok platform={} answer={}", platform, answer);
    reply->set_error(0);
    reply->set_answer(answer);
    return grpc::Status::OK;
}
