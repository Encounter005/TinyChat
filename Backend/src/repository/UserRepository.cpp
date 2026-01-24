#include "UserRepository.h"
#include "common/UserMessage.h"
#include "dao/UserDAO.h"
#include "common/const.h"
#include "infra/LogManager.h"
#include "infra/RedisManager.h"
#include <json/json.h>

// Read operations with caching
Result<UserInfo> UserRepository::getUserById(int uid) {
    auto redisManager = RedisManager::getInstance();

    // 1. 从Redis缓存中获取用户信息
    std::string key = USER_BASE_PREFIX + std::to_string(uid);
    std::string user_info;
    auto        res = redisManager->Get(key, user_info);

    if (res) {
        // 缓存命中
        Json::Reader reader;
        Json::Value  value;
        if (reader.parse(user_info, value)) {
            UserInfo userInfo = UserJsonMapper::FromJson(value);
            LOG_INFO("[Cache] Hit cache for user ID: {}", uid);
            return Result<UserInfo>::OK(userInfo);
        }
    }

    // 2. 缓存未命中，从MySQL中获取
    LOG_INFO("[Cache] Miss cache for user ID: {}, querying MySQL", uid);
    auto dbRes = UserDAO::getInstance()->FindUserByUid(uid);
    if (dbRes.IsOK()) {
        UserInfo userInfo = dbRes.Value();

        // 3. 将用户信息存入Redis缓存
        Json::Value jsonUser = UserJsonMapper::ToJson(userInfo);
        Json::FastWriter writer;
        std::string      jsonStr = writer.write(jsonUser);

        redisManager->Set(key, jsonStr);
        LOG_INFO("[Cache] Set cache for user ID: {}", uid);
        return Result<UserInfo>::OK(userInfo);
    }

    LOG_WARN("[Cache] User with ID: {} not found in MySQL", uid);
    return Result<UserInfo>::Error(dbRes.Error());
}

Result<std::string> UserRepository::getEmailByName(const std::string& name) {
    // This is a pass-through because caching this might not be beneficial
    // unless it's frequently requested and rarely changed.
    return UserDAO::getInstance()->FindEmailByName(name);
}

// Read operation for authentication (bypasses cache for security)
Result<UserInfo> UserRepository::findUserAuthInfoByEmail(
    const std::string& email) {
    // Authentication must always hit the database to get the latest password
    return UserDAO::getInstance()->FindUserByEmail(email);
}

// Write operations with cache invalidation
Result<int> UserRepository::createUser(
    const std::string& name, const std::string& email,
    const std::string& pwd) {
    // Just pass through to DAO. No need to cache on creation,
    // it will be cached on first read.
    return UserDAO::getInstance()->InsertUser(name, email, pwd);
}

Result<void> UserRepository::resetPassword(
    const std::string& name, const std::string& new_pass) {
    auto res = UserDAO::getInstance()->ResetPwd(name, new_pass);
    if (res.IsOK()) {
        // Find user by name to get UID for cache clearing
        auto userRes = UserDAO::getInstance()->FindEmailByName(name);
        if (userRes.IsOK()) {
            auto authRes =
                UserDAO::getInstance()->FindUserByEmail(userRes.Value());
            if (authRes.IsOK()) {
                clearUserCache(authRes.Value().uid);
            }
        }
    }
    return res;
}

// Direct cache manipulation
void UserRepository::clearUserCache(int uid) {
    auto        redisManager = RedisManager::getInstance();
    std::string key          = USER_BASE_PREFIX + std::to_string(uid);
    redisManager->Del(key);
    LOG_INFO("[Cache] Cleared cache for user ID: {}", uid);
}


void UserRepository::UserIpWithServer(const std::string& ip, const std::string &server_name) {
    auto redisManager = RedisManager::getInstance();
    std::string ipkey = USER_IP_PREFIX + ip;
    redisManager->Set(ipkey, server_name);
    LOG_INFO("[RedisManager] ip:{} -> server:{}", ip, server_name);
}
