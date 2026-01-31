#include "RedisManager.h"
#include "ConfigManager.h"
#include "LogManager.h"
#include "RedisConPool.h"
#include "hiredis.h"
#include "read.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

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
    redisReply* reply   = (redisReply*) redisCommand(context, "GET %s", key.c_str());
    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] Get failed: Command error for key: {}", key);
        return false;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        LOG_ERROR(
            "[RedisManager] Wrong type for Key: {}. Expected String, but got "
            "type {}",
            key,
            reply->type);
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
        LOG_ERROR("[RedisManager] Set failed: key: {}", key);
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
    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] LPUSH failed: {} << {}", key, value);
        return false;
    }

    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);

    if (ok) LOG_INFO("[RedisManager] LPUSH success: {} -> {}", key, value);
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
    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] RPUSH failed: {} -> {}", key, value);
        return false;
    }

    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);

    if (ok) LOG_INFO("[RedisManager] RPUSH success: {} -> {}", key, value);
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
    const std::string& hkey, const std::string& key, const std::string& value) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) return false;

    redisReply* reply = (redisReply*) redisCommand(
        context, "HSET %s %s %s", hkey.c_str(), key.c_str(), value.c_str());
    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] {} failed: Command error or connection lost",
            __FUNCTION__);
        return false;
    }

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
    LOG_INFO("[RedisManager] HGET success: {} -> {} = {}", key, hkey, value);
    return value;
}

bool RedisManager::Del(const std::string& key) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) return false;

    redisReply* reply
        = (redisReply*) redisCommand(context, "DEL %s", key.c_str());
    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] Del failed, key is: {}", key);
        return false;
    }

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
    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] Internal error in ExistsKey");
        return false;
    }

    bool exists = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);

    LOG_INFO(
        "[RedisManager] EXISTS check: key {} {}",
        key,
        exists ? "EXISTS" : "NOT FOUND");

    return exists;
}

bool RedisManager::HDel(const std::string& hkey, const std::string& field) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] HDEL failed: No available connection");
        return false;
    }

    // 执行命令
    redisReply* reply = (redisReply*) redisCommand(
        context, "HDEL %s %s", hkey.c_str(), field.c_str());

    // 1. 检查 reply 是否为空（网络错误或连接丢失）
    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] HDEL failed: Command error for hash: {}, field: {}",
            hkey,
            field);
        return false;
    }

    // 2. 检查返回类型是否为整数 (HDEL 返回删除的域的数量)
    bool ok = (reply->type == REDIS_REPLY_INTEGER);

    // 3. 必须释放内存
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO(
            "[RedisManager] HDEL success: hash: {}, field: {}", hkey, field);
    } else {
        LOG_WARN(
            "[RedisManager] HDEL execution issue or field not found: hash: {}, "
            "field: {}",
            hkey,
            field);
    }

    return ok;
}

void RedisManager::Close() {
    _pool->Close();
    LOG_INFO("[RedisManager] Pool closed.");
}

long long RedisManager::HIncrBy(
    const std::string& hkey, const std::string& field, long long delta) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] HINCRBY failed: no available connection");
        return -1;
    }
    redisReply* reply = (redisReply*) redisCommand(
        context, "HINCRBY %s %s %lld", hkey.c_str(), field.c_str(), delta);
    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] HINCRBY wrong reply type: hash={}, field={}, "
            "type={}",
            hkey,
            field,
            reply->type);
        freeReplyObject(reply);
        return -1;
    }
    auto new_value = reply->integer;
    freeReplyObject(reply);
    LOG_INFO(
        "[RedisManager] HINCRBY success: {}[{}] {} => {}",
        hkey,
        field,
        delta,
        new_value);

    return new_value;
}

bool RedisManager::LRange(
    const std::string& key, int start, int stop,
    std::vector<std::string>& values) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) return false;

    redisReply* reply = (redisReply*) redisCommand(
        context, "LRANGE %s %d %d", key.c_str(), start, stop);
    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] LRANGE failed: command error for key: {}", key);
        return false;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        LOG_ERROR(
            "[RedisManager] LRANGE failed: wrong type: {}, expected array",
            reply->type);
        freeReplyObject(reply);
        return false;
    }

    for (size_t i = 0; i < reply->elements; ++i) {
        values.push_back(reply->element[i]->str);
    }
    freeReplyObject(reply);
    LOG_INFO(
        "[RedisManager] LRANGE success: key: {}, count: {}",
        key,
        values.size());
    return true;
}

std::string RedisManager::AcquireLock(
    const std::string& lock_name, int lock_timeout, int acquire_timeout) {
    std::string id       = generateUUID();
    std::string lock_key = "lock:" + lock_name;
    auto        end_time = std::chrono::steady_clock::now()
                    + std::chrono::seconds(acquire_timeout);
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    while (std::chrono::steady_clock::now() < end_time) {
        redisReply* reply = (redisReply*) redisCommand(
            context,
            "SET %s %s NX EX %d",
            lock_key.c_str(),
            id.c_str(),
            lock_timeout);
        if (reply != nullptr) {
            if (reply->type == REDIS_REPLY_STATUS
                && std::string(reply->str) == "OK") {
                freeReplyObject(reply);
                return id;
            }
            freeReplyObject(reply);
        }
        std::this_thread::sleep_for(
            std::chrono::milliseconds(1));   // 防止忙等待
    }

    return "";
}
bool RedisManager::ReleaseLock(
    const std::string& lock_name, const std::string& id) {
    std::string    lock_key = "lock:" + lock_name;
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();

    // Lua Script: 检查锁表示是否匹配，匹配则删除
    const char* lua_script = "if redis.call('get', KEYS[1]) == ARGV[1] then \
                                return redis.call('del', KEYS[1]) \
                             else \
                                return 0 \
                             end";
    redisReply* reply      = (redisReply*) redisCommand(
        context, "EVAL %s 1 %s %s", lua_script, lock_key.c_str(), id.c_str());
    bool success = false;
    if (reply != nullptr) {
        if (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1) {
            success = true;
        }

        freeReplyObject(reply);
    }
    return success;
}

std::string RedisManager::generateUUID() {
    auto uuid = boost::uuids::random_generator()();
    return boost::uuids::to_string(uuid);
}
