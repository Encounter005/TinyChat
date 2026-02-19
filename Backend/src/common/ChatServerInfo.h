#ifndef CHATSERVERINFO_H_
#define CHATSERVERINFO_H_
#include <string>
#include <iostream>

static const std::string LOGIN_COUNT = "chatserver:login:";
static const std::string ACTIVATE = "chatserver:activate:";

struct ChatServerInfo {

    ChatServerInfo() : host(""), port(""), name(""), conn_count(0) {}
    ChatServerInfo(const ChatServerInfo& other) {
        this->host       = other.host;
        this->port       = other.port;
        this->conn_count = other.conn_count;
        this->name       = other.name;
    }
    ChatServerInfo operator=(const ChatServerInfo& other) {
        if (this == &other) {
            return *this;
        }
        this->host       = other.host;
        this->port       = other.port;
        this->conn_count = other.conn_count;
        this->name       = other.name;
        return *this;
    }



    std::string host;
    std::string port;
    std::string name;
    int         conn_count;
    int heartbeat_interval = 45;       // 客户端心跳间隔（秒）
    int heartbeat_check_interval = 5;  // 服务端检查间隔（秒）
    int heartbeat_timeout = 60;        // 心跳超时时间（秒）
    int heartbeat_probe_wait = 5;      // 探测包等待时间（秒）
};

#endif // CHATSERVERINFO_H_
