#ifndef CHATSERVERINFO_H_
#define CHATSERVERINFO_H_
#include <string>
#include <iostream>

static const std::string LOGIN_COUNT = "chatserver:login:";

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
};

#endif // CHATSERVERINFO_H_
