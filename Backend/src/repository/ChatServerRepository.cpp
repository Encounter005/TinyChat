#include "ChatServerRepository.h"
#include "common/ChatServerInfo.h"
#include "infra/RedisManager.h"
#include <cstdlib>


int ChatServerRepository::IncrConnection(const std::string &server_name) {
    return RedisManager::getInstance()->HIncrBy(LOGIN_COUNT, server_name, 1);
}

void ChatServerRepository::DelConnectionCount(const std::string &server_name) {
    RedisManager::getInstance()->HDel(LOGIN_COUNT, server_name);
    LOG_INFO("[RedisManager] Del {}'s connection", server_name);
}

int ChatServerRepository::DecrConnection(const std::string &server_name) {
    return RedisManager::getInstance()->HIncrBy(LOGIN_COUNT, server_name, -1);
}

int ChatServerRepository::GetConnectionCount(const std::string &server_name) {
    auto count_str
        = RedisManager::getInstance()->HGet(LOGIN_COUNT, server_name);
    LOG_INFO("[RedisManager] {}'s Login count is: {}", server_name, count_str);
    return std::atoi(count_str.c_str());
}

void ChatServerRepository::RestConnection(const std::string &server_name) {
    RedisManager::getInstance()->HSet(LOGIN_COUNT, server_name, "0");
}

void ChatServerRepository::ActivateServer(const std::string& server_name) {
    RedisManager::getInstance()->HSet(ACTIVATE, server_name, "1");
}

bool ChatServerRepository::isServerActivated(const std::string& server_name) {
    auto res = RedisManager::getInstance()->HGet(ACTIVATE, server_name);
    return !res.empty();
}

void ChatServerRepository::DeactivateServer(const std::string& server_name) {
    RedisManager::getInstance()->HDel(ACTIVATE, server_name);
}
