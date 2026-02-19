#ifndef CHATCLIENT_H_
#define CHATCLIENT_H_

#include "common/ChatServerInfo.h"
#include "common/UserMessage.h"
#include "common/singleton.h"
#include "infra/ChannelPool.h"
#include "infra/StubFactory.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <json/value.h>

using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::ChatService;
using message::SendChatMsgReq;
using message::SendChatMsgRsp;
using message::TextChatData;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::ChatService;
using message::KickUserReq;
using message::KickUserRsp;


class ChatClient : public SingleTon<ChatClient> {
    friend class SingleTon<ChatClient>;

public:
    ~ChatClient() = default;
    AddFriendRsp NotifyAddFriend(
        const std::string& server_name, const AddFriendReq& req);
    AuthFriendRsp NotifyAuthFriend(
        const std::string& server_name, const AuthFriendReq& req);
    bool GetBaseInfo(
        const std::string& base_key, int uid,
        std::shared_ptr<UserInfo>& userinfo);
    TextChatMsgRsp NotifyTextChatMsg(
        const std::string& server_ip, const TextChatMsgReq& req,
        const Json::Value& res);
    KickUserRsp NotifyKickUser(const std::string& server_name, const KickUserReq& req);

private:
    explicit ChatClient();
private:
    std::unordered_map<std::string, std::shared_ptr<ChannelPool>> _pools;
};


#endif   // CHATCLIENT_H_
