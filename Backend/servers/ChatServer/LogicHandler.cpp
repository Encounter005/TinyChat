#include "LogicHandler.h"
#include "common/const.h"
#include "infra/LogManager.h"
#include "service/UserService.h"
#include "repository/ChatServerRepository.h"
#include "repository/UserRepository.h"
#include "UserManager.h"
#include "grpcClient/StatusClient.h"
#include <json/value.h>
#include <json/reader.h>

void LogicHandler::HelloEcho(std::shared_ptr<Session> session, const Message &msg) {
        LOG_INFO("recv from {}: {}", session->Id(), msg.body);
        session->Send(1, "echo: " + msg.body);
}

void LogicHandler::HandleLogin(const ChatServerInfo& server_info, std::shared_ptr<Session> session, const Message &msg) {
        Json::Reader reader;
        Json::Value  src, root;
        if (!reader.parse(msg.body, src)) {
            LOG_ERROR("[ChatServer] JSON parse failed: {}", msg.body);
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

        auto res = UserService::GetUserBase(uid);
        if (res.IsOK()) {
            root["error"] = static_cast<int>(ErrorCodes::SUCCESS);
            root["token"] = rsp.token();
            root["name"]  = res.Value().name;
        } else {
            root["error"]     = static_cast<int>(res.Error());
            root["error_msg"] = ErrorMsg(res.Error());
        }

        auto old_session = UserManager::getInstance()->GetSession(uid);
        if (old_session) {
            LOG_INFO("User {} already login, kicing old session", uid);
            old_session->PostClose();
        }

        // Bind session with user_uid
        session->SetUid(uid);
        UserManager::getInstance()->Bind(uid, session);
        LOG_INFO("User {} bound to session {}", uid, session->Id());

        // modify connection count
        auto server_name = server_info.name;
        ChatServerRepository::IncrConnection(server_name);

        // save user login's server
        UserRepository::UserIpWithServer(std::to_string(uid), server_name);

        // send the response
        session->Send(MsgId::ID_CHAT_LOGIN_RSP, root.toStyledString());
}
