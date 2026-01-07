#include "RedisManager.h"
#include "ConfigManager.h"
#include "LogManager.h"
#include "RedisConPool.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

RedisManager::RedisManager() {
    auto globalConfig = ConfigManager::getInstance();
    auto host         = (*globalConfig)["Redis"]["host"];
    auto port_str     = (*globalConfig)["Redis"]["port"];
    auto port         = atoi(port_str.c_str());
    auto passwd       = (*globalConfig)["Redis"]["passwd"];
    _pool.reset(new RedisConPool(host.c_str(), port, passwd.c_str(), 5));
    LOG_INFO("[RedisManager] Pool initialized with 5 connections.");
}

RedisManager::~RedisManager() {
    Close();
    LOG_INFO("[RedisManager] Manager destroyed.");
}

bool RedisManager::Get(const std::string& key, std::string& value) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] Get failed: No available connection");
        return false;
    }

    redisReply* reply
        = (redisReply*) redisCommand(context, "GET %s", key.c_str());
    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] Get failed: Command error for key: {}", key);
        return false;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        LOG_ERROR(
            "[RedisManager] Get failed: Key: {} not found or wrong type", key);
        freeReplyObject(reply);
        return false;
    }

    value = reply->str;
    freeReplyObject(reply);
    LOG_INFO("[RedisManager] Get success: {} = {}  ", key, value);
    return true;
}

bool RedisManager::Set(const std::string& key, const std::string& value) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) return false;

    redisReply* reply = (redisReply*) redisCommand(
        context, "SET %s %s", key.c_str(), value.c_str());
    if (reply == nullptr) {
        LOG_INFO("[RedisManager] Set failed: key: {}", key);
        return false;
    }

    bool ok
        = (reply->type == REDIS_REPLY_STATUS
           && strcasecmp(reply->str, "OK") == 0);
    freeReplyObject(reply);


    if (ok)
        LOG_INFO("[RedisManager] Set success: {} = {} ", key, value);
    else
        LOG_ERROR("[RedisManager] Set failed: status error for key: {}", key);
    return ok;
}

bool RedisManager::LPush(const std::string& key, const std::string& value) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) return false;

    redisReply* reply = (redisReply*) redisCommand(
        context, "LPUSH %s %s", key.c_str(), value.c_str());
    if (reply == nullptr) return false;

    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);

    if (ok) LOG_INFO("[RedisManager] LPUSH success: {} << {}", key, value);
    return ok;
}

bool RedisManager::LPop(const std::string& key, std::string& value) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) return false;

    redisReply* reply
        = (redisReply*) redisCommand(context, "LPOP %s", key.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        LOG_ERROR("[RedisManager] LPOP failed: list: {} is empty", key);
        return false;
    }

    value = reply->str;
    freeReplyObject(reply);
    LOG_INFO("[RedisManager] LPOP success: {} >> {}", key, value);
    return true;
}

bool RedisManager::RPush(const std::string& key, const std::string& value) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) return false;

    redisReply* reply = (redisReply*) redisCommand(
        context, "RPUSH %s %s", key.c_str(), value.c_str());
    if (reply == nullptr) return false;

    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);

    if (ok) LOG_INFO("[RedisManager] RPUSH success: {} >> {}", key, value);
    return ok;
}

bool RedisManager::RPop(const std::string& key, std::string& value) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) return false;

    redisReply* reply
        = (redisReply*) redisCommand(context, "RPOP %s", key.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        LOG_ERROR("[RedisManager] RPOP failed: list: {} is empty", key);
        return false;
    }

    value = reply->str;
    freeReplyObject(reply);
    LOG_INFO("[RedisManager] RPOP success: {} << {}", key, value);
    return true;
}

bool RedisManager::HSet(
    const std::string& key, const std::string& hkey, const std::string& value) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) return false;

    redisReply* reply = (redisReply*) redisCommand(
        context, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
    if (reply == nullptr) return false;

    bool ok = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);
    if (ok)
        LOG_INFO(
            "[RedisManager] HSET success: {} -> {} = {} ", key, hkey, value);
    return ok;
}

std::string RedisManager::HGet(
    const std::string& key, const std::string& hkey) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) return "";

    redisReply* reply = (redisReply*) redisCommand(
        context, "HGET %s %s", key.c_str(), hkey.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        LOG_ERROR(
            "[RedisManager] HGET failed: field: {} not in hash: {}", hkey, key);
        return "";
    }

    std::string value = reply->str;
    freeReplyObject(reply);
    LOG_INFO("[RedisManager] HGET success: {} -> {} = ", key, hkey, value);
    return value;
}

bool RedisManager::Del(const std::string& key) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) return false;

    redisReply* reply
        = (redisReply*) redisCommand(context, "DEL %s", key.c_str());
    if (reply == nullptr) return false;

    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);

    if (ok)
        LOG_INFO("[RedisManager] DEL success: key: {}", key);
    else
        LOG_INFO("[RedisManager] DEL ignored: key: {} did not exist", key);
    return ok;
}

bool RedisManager::ExistsKey(const std::string& key) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) return false;

    redisReply* reply
        = (redisReply*) redisCommand(context, "EXISTS %s", key.c_str());
    if (reply == nullptr) return false;

    bool exists = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);

    LOG_INFO(
        "[RedisManager] EXISTS check: key {} {}",
        key,
        exists ? "EXISTS" : "NOT FOUND");

    return exists;
}

void RedisManager::Close() {
    _pool->Close();
    LOG_INFO("[RedisManager] Pool closed.");
}
