#include "ChatServiceImpl.h"


ChatServiceImpl::ChatServiceImpl() {}

Status ChatServiceImpl::NotifyAddFriend(
    ServerContext* context, const AddFriendReq* request, AddFriendRsp* reply) {
    return Status::OK;
}
Status ChatServiceImpl::NotifyAuthFriend(
    ServerContext* context, const AuthFriendReq* request,
    AuthFriendRsp* reply) {
    return Status::OK;
}
Status ChatServiceImpl::NotifyTextChatmsg(
    ServerContext* context, const TextChatMsgReq* request,
    TextChatMsgRsp* response) {
    return Status::OK;
}


bool ChatServiceImpl::GetBaseInfo(
    const std::string& base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
    return true;
}
