#ifndef CHATSERVICEIMPL_H_
#define CHATSERVICEIMPL_H_

#include "common/UserMessage.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/server_context.h>
using grpc::ServerContext;
using grpc::Status;
using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::ChatService;
using message::KickUserReq;
using message::KickUserRsp;
using message::SendChatMsgReq;
using message::SendChatMsgRsp;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;

class ChatServiceImpl : public ChatService::Service {
public:
    ChatServiceImpl();
    Status NotifyAddFriend(
        ServerContext* context, const AddFriendReq* request,
        AddFriendRsp* reply) override;
    Status NotifyAuthFriend(
        ServerContext* context, const AuthFriendReq* request,
        AuthFriendRsp* reply) override;
    Status NotifyTextChatmsg(
        ServerContext* context, const TextChatMsgReq* request,
        TextChatMsgRsp* response) override;
    Status NotifyKickUser(
        ServerContext* context, const KickUserReq* request,
        KickUserRsp* response) override;

private:
};


#endif   // CHATSERVICEIMPL_H_
