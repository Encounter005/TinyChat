#ifndef LOGICHANDLER_H_
#define LOGICHANDLER_H_

#include "Messsage.h"
#include "common/ChatServerInfo.h"
#include "session.h"
#include <json/value.h>
#include <memory>
class ChatServer;
class LogicHandler {
public:
    static void HelloEcho(std::shared_ptr<Session> session, const Message& msg);
    static void HandleLogin(
        const ChatServerInfo& server_info, std::shared_ptr<Session> session,
        const Message& msg);
    static void HandleSearch(
        std::shared_ptr<Session> session, const Message& msg);
    static void AddFriendApply(
        const ChatServerInfo& server_info, std::shared_ptr<Session> session,
        const Message& msg);
    static void AuthFriendApply(
        const ChatServerInfo& server_info, std::shared_ptr<Session> session,
        const Message& msg);
    static void HandleChatTextMsg(
        const ChatServerInfo& server_info, std::shared_ptr<Session> session,
        const Message& msg);

private:
    static bool ParseJson(const std::string& data, Json::Value& root);
};


#endif   // LOGICHANDLER_H_
