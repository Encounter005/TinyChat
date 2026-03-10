#include "ChatServiceImpl.h"
#include "UserManager.h"
#include "common/const.h"
#include "grpcClient/StatusClient.h"
#include "infra/Defer.h"
#include "message.pb.h"
#include "repository/UserRepository.h"
#include "service/UserService.h"
#include <ctime>
#include <json/value.h>


ChatServiceImpl::ChatServiceImpl() {}

Status ChatServiceImpl::NotifyAddFriend(
    ServerContext* context, const AddFriendReq* request, AddFriendRsp* reply) {
    auto  touid = request->touid();
    Defer defer([request, reply]() {
        reply->set_error(ErrorCode::SUCCESS);
        reply->set_applyuid(request->applyuid());
        reply->set_touid(request->touid());
    });

    auto session = UserManager::getInstance()->GetSession(touid);
    if (session == nullptr) {
        return Status::OK;
    }

    Json::Value root;
    root["error"]    = static_cast<int>(ErrorCodes::SUCCESS);
    root["applyuid"] = request->applyuid();
    root["name"]     = request->name();
    root["desc"]     = request->desc();
    root["icon"]     = request->icon();
    root["sex"]      = request->sex();
    root["nick"]     = request->nick();

    session->Send(MsgId::ID_ADD_FRIEND_REQ, root.toStyledString());
    return Status::OK;
}
Status ChatServiceImpl::NotifyAuthFriend(
    ServerContext* context, const AuthFriendReq* request,
    AuthFriendRsp* reply) {

    auto  touid   = request->touid();
    auto  fromuid = request->fromuid();
    auto  session = UserManager::getInstance()->GetSession(touid);
    Defer defer([request, reply]() {
        reply->set_error(ErrorCode::SUCCESS);
        reply->set_fromuid(request->fromuid());
        reply->set_touid(request->touid());
    });

    if (session == nullptr) {
        return Status::OK;
    }

    Json::Value root;
    root["fromuid"]   = fromuid;
    root["touid"]     = touid;
    auto res_userinfo = UserService::GetUserBase(fromuid);

    if (res_userinfo.IsOK()) {
        auto user_info = res_userinfo.Value();
        root["error"]  = static_cast<int>(ErrorCodes::SUCCESS);
        root["name"]   = user_info.name;
        root["nick"]   = user_info.nick;
        root["icon"]   = user_info.icon;
        root["sex"]    = user_info.sex;
    } else {
        root["error"] = static_cast<int>(ErrorCodes::UID_INVALID);
    }

    session->Send(MsgId::ID_NOTIFY_AUTH_FRIEND_REQ, root.toStyledString());
    return Status::OK;
}
Status ChatServiceImpl::NotifyTextChatmsg(
    ServerContext* context, const TextChatMsgReq* request,
    TextChatMsgRsp* response) {
    auto touid   = request->touid();
    auto session = UserManager::getInstance()->GetSession(touid);
    response->set_error(ErrorCode::SUCCESS);
    if (session == nullptr) {
        return Status::OK;
    }

    Json::Value root;
    root["error"]   = ErrorCode::SUCCESS;
    root["fromuid"] = request->fromuid();
    root["touid"]   = request->touid();
    root["timestamp"] = static_cast<int64_t>(std::time(nullptr));

    Json::Value text_array;
    for (auto& msg : request->textmsgs()) {
        Json::Value element;
        element["content"] = msg.msgcontent();
        element["msgid"]   = msg.msgid();
        text_array.append(element);
    }

    root["text_array"] = text_array;

    session->Send(MsgId::ID_NOTIFY_TEXT_CHAT_MSG_REQ, root.toStyledString());

    return Status::OK;
}

Status ChatServiceImpl::NotifyKickUser(
    ServerContext* context, const KickUserReq* request, KickUserRsp* response) {

    auto uid = request->uid();
    LOG_INFO("[ChatServiceImpl] Received kick user request for uid: {}", uid);

    Defer defer([request, response]() {
        response->set_error(ErrorCode::SUCCESS);
        response->set_uid(request->uid());
    });

    UserManager::getInstance()->KickUser(
        uid, "Your account has been logged in from another device");
    LOG_INFO("[ChatServiceImpl] User {} kicked successfully", uid);
    return Status::OK;
}


Status ChatServiceImpl::NotifyUserIcon(
    ServerContext*,
    const message::UserIconReq* request,
    message::UserIconRsp* response) {

    const int ownerUid = request->owner_uid();
    const int changedUid = request->uid();

    response->set_owner_uid(ownerUid);
    response->set_uid(changedUid);
    response->set_error(message::ErrorCode::SUCCESS);

    auto session = UserManager::getInstance()->GetSession(ownerUid);
    if (!session) {
        // owner 不在线：通知不做离线存储（离线靠下次登录 friend_list 获取新 icon）
        return Status::OK;
    }

    Json::Value root;
    root["uid"] = changedUid;
    root["icon"] = request->icon();

    session->Send(MsgId::ID_NOTIFY_USER_ICON_REQ, root.toStyledString());
    return Status::OK;
}
