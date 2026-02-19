#include "LogicHandler.h"
#include "ChatServiceImpl.h"
#include "SessionManager.h"
#include "UserManager.h"
#include "common/ChatServerInfo.h"
#include "common/UserMessage.h"
#include "common/const.h"
#include "grpcClient/ChatClient.h"
#include "grpcClient/StatusClient.h"
#include "infra/Defer.h"
#include "infra/DistLock.h"
#include "infra/LogManager.h"
#include "infra/RedisManager.h"
#include "repository/ChatServerRepository.h"
#include "repository/MessagePersistenceRepository.h"
#include "repository/UserRepository.h"
#include "service/UserService.h"
#include <json/reader.h>
#include <json/value.h>
#include <memory>

void LogicHandler::HelloEcho(
    std::shared_ptr<Session> session, const Message &msg) {
    LOG_INFO("recv from {}: {}", session->Id(), msg.body);
    session->Send(1, "echo: " + msg.body);
}

bool LogicHandler::ParseJson(const std::string &data, Json::Value &root) {
    Json::Reader reader;
    if (!reader.parse(data, root)) {
        LOG_ERROR("[ChatServer] JSON parse failed: {}", data);
        return false;
    }
    return true;
}


void LogicHandler::HandleLogin(
    const ChatServerInfo &server_info, std::shared_ptr<Session> session,
    const Message &msg) {
    Json::Value src, root;
    if (!ParseJson(msg.body, src)) {
        return;
    }
    auto uid   = src["uid"].asInt();
    auto token = src["token"].asString();
    LOG_INFO("[ChatServer] user login uid is: {}, token is {}", uid, token);
    auto rsp = StatusClient::getInstance()->Login(uid, token);
    if (rsp.error() != ErrorCode::SUCCESS) {
        LOG_WARN("[ChatServer] token error");
        return;
    }


    std::string lock_name = "user:kick:" + std::to_string(uid);
    DistLock    lock(lock_name, 10, 5);

    if (!lock.isLocked()) {
        LOG_ERROR("[ChatServer] Failed to acquire lock for user {}", uid);
    }

    auto res_server_name = UserRepository::FindUserIpServerByUid(uid);
    if (res_server_name.IsOK()) {
        std::string old_server = res_server_name.Value();
        if (old_server == server_info.name) {
            // 场景1：用户在当前服务器重复登录
            LOG_INFO(
                "[ChatServer] User {} already on this server, kicking old "
                "session",
                uid);
            UserManager::getInstance()->KickUser(
                uid, "Your account has been logged in from another device");
        } else {
            // 场景2：用户在其他服务器，需要跨服务器踢人
            LOG_INFO(
                "[ChatServer] User {} on server {}, kicking via RPC",
                uid,
                old_server);

            message::KickUserReq kick_req;
            kick_req.set_uid(uid);

            auto kick_rsp = ChatClient::getInstance()->NotifyKickUser(
                old_server, kick_req);

            if (kick_rsp.error() != ErrorCode::SUCCESS) {
                LOG_WARN(
                    "[ChatServer] Failed to kick user {} from server {} "
                    "(continuing login)",
                    uid,
                    old_server);
                // RPC 失败仍然继续登录，避免因网络问题影响用户体验
            } else {
                LOG_INFO(
                    "[ChatServer] Successfully kicked user {} from server {}",
                    uid,
                    old_server);
            }
        }
    } else {
        // 场景3：用户首次登录或 Redis 中没有记录
        LOG_INFO(
            "[ChatServer] User {} not found in any server, new login", uid);
    }


    auto res = UserService::GetUserBase(uid);
    if (res.IsOK()) {
        root["error"] = static_cast<int>(ErrorCodes::SUCCESS);
        root["token"] = rsp.token();
        root["name"]  = res.Value().name;
        root["icon"]  = res.Value().icon;
        root["nick"]  = res.Value().nick;
        root["uid"]   = res.Value().uid;
    } else {
        root["error"]     = static_cast<int>(res.Error());
        root["error_msg"] = ErrorMsg(res.Error());
    }


    auto apply_list_res = UserService::GetApplyList(uid);
    auto apply_list     = apply_list_res.Value();
    if (!apply_list.empty()) {
        for (auto &apply : apply_list) {
            Json::Value obj;
            obj["name"]   = apply->_name;
            obj["uid"]    = apply->_uid;
            obj["icon"]   = apply->_icon;
            obj["nick"]   = apply->_nick;
            obj["sex"]    = apply->_sex;
            obj["desc"]   = apply->_desc;
            obj["status"] = apply->_status;
            root["apply_list"].append(obj);
        }
    }

    auto friend_list_res = UserService::GetFriendList(uid);
    auto friendList      = friend_list_res.Value();
    if (!friendList.empty()) {
        LOG_DEBUG("friendList is not empty");
        for (auto &friend_info : friendList) {
            Json::Value obj;
            obj["name"] = friend_info->name;
            obj["uid"]  = friend_info->uid;
            obj["icon"] = friend_info->icon;
            obj["nick"] = friend_info->nick;
            obj["sex"]  = friend_info->sex;
            obj["desc"] = friend_info->desc;
            obj["back"] = friend_info->back;
            root["friend_list"].append(obj);
        }
    }



    // Bind session with user_uid
    session->SetUid(uid);
    UserManager::getInstance()->Bind(uid, session);
    LOG_INFO("User {} bound to session {}", uid, session->Id());

    // modify connection count
    auto server_name = server_info.name;
    session->MarkAsLoggedIn(server_info.name);

    // save user login's server
    UserRepository::BindUserIpWithServer(uid, server_name);


    // send the response
    MsgId id = static_cast<MsgId>(msg.msg_id);
    session->Send(ReqToRsp(id), root.toStyledString());

    // Check offline messages
    auto offline_msgs_res = UserRepository::GetOfflineMessages(uid);
    if (offline_msgs_res.IsOK()) {
        auto msgs = offline_msgs_res.Value();
        for (const auto &msg_body : msgs) {
            Message offline_msg;
            offline_msg.msg_id
                = static_cast<uint16_t>(MsgId::ID_NOTIFY_TEXT_CHAT_MSG_REQ);
            offline_msg.body = msg_body;
            LogicHandler::HandleChatTextMsg(server_info, session, offline_msg);
        }
    }
}

void LogicHandler::HandleSearch(
    std::shared_ptr<Session> session, const Message &msg) {
    Json::Value src, root;
    if (!ParseJson(msg.body, src)) {
        return;
    }

    auto uid = src["uid"].asInt();
    LOG_INFO("[ChatServer] recv user uid: {}", uid);
    auto res = UserService::GetUserBase(uid);
    if (res.IsOK()) {
        auto userInfo = res.Value();
        root["uid"]   = userInfo.uid;
        root["name"]  = userInfo.name;
        root["email"] = userInfo.email;
        root["error"] = static_cast<int>(ErrorCodes::SUCCESS);
    } else {
        root["error"] = static_cast<int>(ErrorCodes::UID_INVALID);
    }
    MsgId id = static_cast<MsgId>(msg.msg_id);
    session->Send(ReqToRsp(id), root.toStyledString());
}

void LogicHandler::AddFriendApply(
    const ChatServerInfo &server_info, std::shared_ptr<Session> session,
    const Message &msg) {
    Json::Value src, root;

    if (!ParseJson(msg.body, src)) {
        return;
    }

    auto uid       = src["uid"].asInt();
    auto touid     = src["touid"].asInt();
    auto bakname   = src["bakname"].asString();
    auto applyname = src["applyname"].asString();

    LOG_INFO(
        "[ChatServer] Friend add: from is {}:{}, to is {}:{}",
        uid,
        applyname,
        touid,
        bakname);

    auto res = UserService::AddFriendApply(uid, touid);
    if (!res.IsOK()) {
        MsgId id      = static_cast<MsgId>(msg.msg_id);
        root["error"] = ErrorMsg(res.Error());
        session->Send(ReqToRsp(id), root.toStyledString());
        return;
    }
    // 通知对方
    // 1. 先查看对方在哪个session
    std::string server_name     = "";
    auto        res_server_name = UserRepository::FindUserIpServerByUid(touid);
    if (!res_server_name.IsOK()) {
        return;
    }
    server_name = res_server_name.Value();

    if (server_name == server_info.name) {
        auto res_session = UserManager::getInstance()->GetSession(touid);
        if (res_session) {
            LOG_INFO("[ChatServer] Find res_session");
            root["error"]    = ErrorMsg(ErrorCodes::SUCCESS);
            root["applyuid"] = uid;
            root["name"]     = applyname;
            MsgId id         = static_cast<MsgId>(msg.msg_id);
            res_session->Send(ReqToRsp(id), root.toStyledString());
        }
        return;
    }

    auto         userInfo   = UserRepository::getUserById(uid);
    auto         apply_info = std::make_shared<UserInfo>();
    AddFriendReq add_req;
    add_req.set_name(userInfo.Value().name);
    add_req.set_touid(touid);
    add_req.set_applyuid(uid);
    add_req.set_desc("");
    if (userInfo.IsOK()) {
        add_req.set_icon(apply_info->icon);
        add_req.set_sex(apply_info->sex);
        add_req.set_nick(apply_info->nick);
    }

    ChatClient::getInstance()->NotifyAddFriend(server_name, add_req);
}

void LogicHandler::AuthFriendApply(
    const ChatServerInfo &server_info, std::shared_ptr<Session> session,
    const Message &msg) {
    Json::Value src, root;

    if (!ParseJson(msg.body, src)) {
        return;
    }

    auto uid       = src["fromuid"].asInt();
    auto touid     = src["touid"].asInt();
    auto back_name = src["back"].asString();
    LOG_INFO("[ChatServer] from {} auth friend to {}", uid, touid);

    auto user_info = std::make_shared<UserInfo>();
    auto res       = UserService::GetUserBase(touid);
    if (res.IsOK()) {
        root["error"] = static_cast<int>(ErrorCodes::SUCCESS);
        root["name"]  = res.Value().name;
        root["nick"]  = res.Value().nick;
        root["icon"]  = res.Value().icon;
        root["sex"]   = res.Value().sex;
        root["uid"]   = touid;
    } else {
        root["error"] = static_cast<int>(ErrorCodes::UID_INVALID);
    }

    Defer defer([&root, session, &msg]() {
        auto id = static_cast<MsgId>(msg.msg_id);
        session->Send(ReqToRsp(id), root.toStyledString());
    });

    UserService::AuthFriendApply(uid, touid);
    UserService::AddFriend(uid, touid, back_name);

    // 通知对方
    std::string server_name     = "";
    auto        res_server_name = UserRepository::FindUserIpServerByUid(touid);
    if (!res_server_name.IsOK()) {
        return;
    }
    server_name = res_server_name.Value();

    if (server_name == server_info.name) {
        auto res_session = UserManager::getInstance()->GetSession(touid);
        if (res_session) {
            Json::Value notify;
            notify["fromuid"] = uid;
            notify["touid"]   = touid;
            auto user_info    = UserService::GetUserBase(uid);
            if (user_info.IsOK()) {
                notify["error"] = static_cast<int>(ErrorCodes::SUCCESS);
                notify["name"]  = user_info.Value().name;
                notify["nick"]  = user_info.Value().nick;
                notify["icon"]  = user_info.Value().icon;
                notify["sex"]   = user_info.Value().sex;
            } else {
                notify["error"] = static_cast<int>(ErrorCodes::UID_INVALID);
            }
            auto id = static_cast<MsgId>(msg.msg_id);
            session->Send(
                MsgId::ID_NOTIFY_AUTH_FRIEND_REQ, notify.toStyledString());
        }
        return;
    }

    AuthFriendReq auth_req;
    auth_req.set_fromuid(uid);
    auth_req.set_touid(touid);
    ChatClient::getInstance()->NotifyAuthFriend(server_name, auth_req);
}

void LogicHandler::HandleChatTextMsg(
    const ChatServerInfo &server_info, std::shared_ptr<Session> session,
    const Message &msg) {
    Json::Value src, root;

    if (!ParseJson(msg.body, src)) {
        return;
    }

    auto              uid    = src["fromuid"].asInt();
    auto              touid  = src["touid"].asInt();
    const Json::Value arrays = src["text_array"];
    root["error"]            = static_cast<int>(ErrorCodes::SUCCESS);
    root["text_array"]       = arrays;
    root["fromuid"]          = uid;
    root["touid"]            = touid;

    // Cache Messages
    auto cache_res = MessagePersistenceRepository::SaveChatMessage(
        uid, touid, root.toStyledString());
    
    if (!cache_res.IsOK()) {
        LOG_WARN("[ChatServer] Failed to save message to cache: {} -> {}", uid, touid);
        // 继续处理，缓存失败不应影响消息推送
    } else {
        LOG_DEBUG("[ChatServer] Message cached: {} -> {}", uid, touid);
    }

    Defer defer([&root, session, &msg]() {
        auto id = static_cast<MsgId>(msg.msg_id);
        session->Send(ReqToRsp(id), root.toStyledString());
    });

    // 通知对方
    // 1. 先查看对方在哪个session
    std::string server_name     = "";
    auto        res_server_name = UserRepository::FindUserIpServerByUid(touid);
    if (!res_server_name.IsOK()) {
        // User not logged in (offline)
        UserRepository::SaveOfflineMessage(touid, root.toStyledString());
        return;
    }


    server_name = res_server_name.Value();
    LOG_DEBUG("server_name is {}", server_name);
    if (server_name == server_info.name) {
        auto res_session = UserManager::getInstance()->GetSession(touid);
        if (res_session) {
            auto id = static_cast<MsgId>(msg.msg_id);
            res_session->Send(
                MsgId::ID_NOTIFY_TEXT_CHAT_MSG_REQ, root.toStyledString());
        } else {
            // User mapped to this server but no session
            // (inconsistent/offline)
            UserRepository::SaveOfflineMessage(touid, root.toStyledString());
        }
        return;
    }

    TextChatMsgReq text_msg_req;
    text_msg_req.set_fromuid(uid);
    text_msg_req.set_touid(touid);
    for (const auto &text_obj : arrays) {
        auto content = text_obj["content"].asString();
        auto msgid   = text_obj["msgid"].asString();
        LOG_INFO("msgid is: {}, content is {}", content, msgid);
        auto *text_msg = text_msg_req.add_textmsgs();
        text_msg->set_msgid(msgid);
        text_msg->set_msgcontent(content);
    }


    ChatClient::getInstance()->NotifyTextChatMsg(
        server_name, text_msg_req, root);
}

void LogicHandler::HandleHeartBeat(std::shared_ptr<Session> session, const Message& msg) {
    session->OnHeartBeatRequest();

    Json::Value root;
    root["error"] = static_cast<int>(ErrorCodes::SUCCESS);
    root["timestamp"] = static_cast<int64_t>(std::time(nullptr));
    session->Send(MsgId::ID_HEARTBEAT_RSP, root.toStyledString());
}
