#include "UserRepository.h"
#include "common/UserMessage.h"
#include "common/const.h"
#include "dao/UserDAO.h"
#include "infra/LogManager.h"
#include "infra/RedisManager.h"
#include <json/json.h>
#include <memory>

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
        Json::Value      jsonUser = UserJsonMapper::ToJson(userInfo);
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
    auto        redisManager = RedisManager::getInstance();
    std::string key          = NAME_EMAIL_PREFIX + name;
    std::string email;
    auto        res = redisManager->Get(key, email);
    if (res) {
        LOG_INFO("[Cache] Hit cache for name: {}", name);
        return Result<std::string>::OK(email);
    }

    auto dbRes = UserDAO::getInstance()->FindEmailByName(name);
    if (dbRes.IsOK()) {
        redisManager->Set(key, dbRes.Value());
    }
    return dbRes;
}

// Read operation for authentication (bypasses cache for security)
Result<UserInfo> UserRepository::findUserAuthInfoByEmail(
    const std::string& email) {
    // Authentication must always hit the database to get the latest password
    return UserDAO::getInstance()->FindUserByEmail(email);
}

// Write operations with cache invalidation
Result<int> UserRepository::createUser(
    const std::string& name, const std::string& email, const std::string& pwd) {
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
            auto authRes
                = UserDAO::getInstance()->FindUserByEmail(userRes.Value());
            if (authRes.IsOK()) {
                clearUserCache(authRes.Value().uid);
            }
        }
    }
    return res;
}

Result<void> UserRepository::AddFriendApply(const int& from, const int& to) {
    auto res = UserDAO::getInstance()->AddFriendApply(from, to);
    if (res.IsOK()) {
        auto        redisManager = RedisManager::getInstance();
        std::string key          = FRIEND_APPLY_PREFIX + std::to_string(to);
        redisManager->Del(key);
    }
    return res;
}

Result<void> UserRepository::AddFriend(
    const int& from, const int& to, const std::string& back_name) {
    auto res = UserDAO::getInstance()->AddFriend(from, to, back_name);
    if (res.IsOK()) {
        auto        redisManager = RedisManager::getInstance();
        std::string key1         = FRIEND_LIST_PREFIX + std::to_string(from);
        std::string key2         = FRIEND_LIST_PREFIX + std::to_string(to);
        redisManager->Del(key1);
        redisManager->Del(key2);
    }
    return res;
}

Result<void> UserRepository::AuthFriendApply(const int& from, const int& to) {
    auto res = UserDAO::getInstance()->AuthFriendApply(from, to);
    if (res.IsOK()) {
        auto        redisManager = RedisManager::getInstance();
        std::string key          = FRIEND_APPLY_PREFIX + std::to_string(to);
        redisManager->Del(key);
    }
    return res;
}

Result<std::vector<std::shared_ptr<ApplyInfo>>> UserRepository::GetApplyList(
    int uid) {
    auto        redisManager = RedisManager::getInstance();
    std::string key          = FRIEND_APPLY_PREFIX + std::to_string(uid);
    std::string list_str;
    auto        res = redisManager->Get(key, list_str);
    if (res) {
        Json::Reader reader;
        Json::Value  root;
        if (reader.parse(list_str, root) && root.isArray()) {
            std::vector<std::shared_ptr<ApplyInfo>> applyList;
            for (const auto& item : root) {
                applyList.push_back(std::make_shared<ApplyInfo>(
                    ApplyInfoJsonMapper::FromJson(item)));
            }
            LOG_INFO("[Cache] Hit cache for apply list: {}", uid);
            return Result<std::vector<std::shared_ptr<ApplyInfo>>>::OK(
                applyList);
        }
    }

    auto dbRes = UserDAO::getInstance()->GetApplyList(uid, 0, 10);
    if (dbRes.IsOK()) {
        Json::Value root(Json::arrayValue);
        for (const auto& item : dbRes.Value()) {
            root.append(ApplyInfoJsonMapper::ToJson(*item));
        }
        Json::FastWriter writer;
        redisManager->Set(key, writer.write(root));
    }
    return dbRes;
}

Result<std::string> UserRepository::FindUserIpServerByUid(const int& uid) {
    auto        redisManager = RedisManager::getInstance();
    std::string        key          = USER_IP_PREFIX + std::to_string(uid);
    std::string servername   = "";
    redisManager->Get(key, servername);
    if (!servername.empty()) {
        return Result<std::string>::OK(servername);
    } else {
        return Result<std::string>::Error(ErrorCodes::REDIS_ERROR);
    }
}

using ArrayUserInfo = std::vector<std::shared_ptr<UserInfo>>;
Result<ArrayUserInfo> UserRepository::GetFriendList(int uid) {
    auto        redisManager = RedisManager::getInstance();
    std::string key          = FRIEND_LIST_PREFIX + std::to_string(uid);
    std::string list_str;
    auto        res = redisManager->Get(key, list_str);
    if (res) {
        Json::Reader reader;
        Json::Value  root;
        if (reader.parse(list_str, root) && root.isArray()) {
            ArrayUserInfo friendList;
            for (const auto& item : root) {
                friendList.push_back(std::make_shared<UserInfo>(
                    UserJsonMapper::FromJson(item)));
            }
            LOG_INFO("[Cache] Hit cache for friend list: {}", uid);
            return Result<ArrayUserInfo>::OK(friendList);
        }
    }

    auto dbRes = UserDAO::getInstance()->GetFriendList(uid);
    if (dbRes.IsOK()) {
        Json::Value root(Json::arrayValue);
        for (const auto& item : dbRes.Value()) {
            root.append(UserJsonMapper::ToJson(*item));
        }
        Json::FastWriter writer;
        redisManager->Set(key, writer.write(root));
    }
    return dbRes;
}


// Direct cache manipulation
void UserRepository::clearUserCache(int uid) {
    auto        redisManager = RedisManager::getInstance();
    std::string key          = USER_BASE_PREFIX + std::to_string(uid);
    redisManager->Del(key);
    LOG_INFO("[Cache] Cleared cache for user ID: {}", uid);
}


void UserRepository::BindUserIpWithServer(
    const int& uid, const std::string& server_name) {
    auto        redisManager = RedisManager::getInstance();
    std::string key          = USER_IP_PREFIX + std::to_string(uid);
    redisManager->Set(key, server_name);
    LOG_INFO("[RedisManager] ip:{} -> server:{}", uid, server_name);
}

void UserRepository::UnBindUserIpWithServer(const int& uid) {
    auto        redisManager = RedisManager::getInstance();
    std::string key          = USER_IP_PREFIX + std::to_string(uid);
    redisManager->Del(key);
    LOG_INFO("[RedisManager] Del ip:{}", uid);
}

Result<void> UserRepository::SaveOfflineMessage(int uid, const std::string& msg) {
    auto redisManager = RedisManager::getInstance();
    std::string key = OFFLINE_MSG_PREFIX + std::to_string(uid);
    if (redisManager->RPush(key, msg)) {
        return Result<void>::OK();
    }
    return Result<void>::Error(ErrorCodes::REDIS_ERROR);
}

Result<std::vector<std::string>> UserRepository::GetOfflineMessages(int uid) {
    auto redisManager = RedisManager::getInstance();
    std::string key = OFFLINE_MSG_PREFIX + std::to_string(uid);
    std::vector<std::string> values;
    if (redisManager->LRange(key, 0, -1, values)) {
        redisManager->Del(key);
        return Result<std::vector<std::string>>::OK(values);
    }
    return Result<std::vector<std::string>>::Error(ErrorCodes::REDIS_ERROR);
}
