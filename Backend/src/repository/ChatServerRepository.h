#ifndef CHATSERVERREPOSITORY_H_
#define CHATSERVERREPOSITORY_H_

#include "common/ChatServerInfo.h"
#include "common/singleton.h"
class ChatServerRepository : public SingleTon<ChatServerRepository> {
    friend class SingleTon<ChatServerRepository>;

public:
    static void RestConnection(const std::string& server_name);
    static void DelConnectionCount(const std::string& server_name);
    static int  GetConnectionCount(const std::string& server_name);
    static int IncrConnection(const std::string& server_name);
    static int DecrConnection(const std::string& server_name);
    static void ActivateServer(const std::string& server_name);
    static void DeactivateServer(const std::string& server_name);
    static bool isServerActivated(const std::string& server_name);

private:
};

#endif   // CHATSERVERREPOSITORY_H_
