#ifndef AICHATCLIENT_H_
#define AICHATCLIENT_H_

#include "ai.grpc.pb.h"
#include "ai.pb.h"
#include "common/singleton.h"
#include "infra/ChannelPool.h"
#include <memory>

using ai::AiChatReq;
using ai::AiChatRsp;

class AiChatClient : public SingleTon<AiChatClient> {
    friend class SingleTon<AiChatClient>;

public:
    ~AiChatClient() = default;
    AiChatRsp Chat(
        int uid, const std::string& query,
        const std::vector<std::pair<std::string, std::string&>>& history);

private:
    explicit AiChatClient();
    std::shared_ptr<ChannelPool> _pool;
};



#endif   // AICHATCLIENT_H_
