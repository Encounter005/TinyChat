#ifndef LOGICHANDLER_H_
#define LOGICHANDLER_H_

#include "Messsage.h"
#include "common/ChatServerInfo.h"
#include "session.h"
#include <memory>
class ChatServer;
class LogicHandler {
public:
    static void HelloEcho(std::shared_ptr<Session> session, const Message& msg);
    static void HandleLogin(const ChatServerInfo& server_info, std::shared_ptr<Session> session, const Message& msg);
};


#endif   // LOGICHANDLER_H_
