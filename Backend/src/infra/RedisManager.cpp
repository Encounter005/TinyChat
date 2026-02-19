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
    redisReply*    reply
        = (redisReply*) redisCommand(context, "GET %s", key.c_str());
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

bool RedisManager::Expire(const std::string& key, int seconds) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] EXPIRE failed: no available connection");
        return false;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "EXPIRE %s %d", key.c_str(), seconds);

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] EXPIRE failed: command error for key: {}", key);
        return false;
    }

    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO(
            "[RedisManager] EXPIRE success: key={}, seconds={}", key, seconds);
    } else {
        LOG_WARN(
            "[RedisManager] EXPIRE failed: key={} does not exist or already "
            "has no expiry",
            key);
    }

    return ok;
}

std::map<std::string, std::string> RedisManager::HGetAll(
    const std::string& key) {
    RedisConnGuard                     guard(_pool.get());
    redisContext*                      context = guard.get();
    std::map<std::string, std::string> result;

    if (!context) {
        LOG_ERROR("[RedisManager] HGETALL failed: no available connection");
        return result;
    }

    redisReply* reply
        = (redisReply*) redisCommand(context, "HGETALL %s", key.c_str());

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] HGETALL failed: command error for key: {}", key);
        return result;
    }

    if (reply->type == REDIS_REPLY_NIL) {
        LOG_DEBUG("[RedisManager] HGETALL: key={} does not exist", key);
        freeReplyObject(reply);
        return result;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        LOG_ERROR(
            "[RedisManager] HGETALL failed: wrong type: {}, expected array",
            reply->type);
        freeReplyObject(reply);
        return result;
    }

    // HGETALL 返回数组，格式为 [field1, value1, field2, value2, ...]
    for (size_t i = 0; i < reply->elements; i += 2) {
        if (i + 1 < reply->elements) {
            std::string field = reply->element[i]->str;
            std::string value = reply->element[i + 1]->str;
            result[field]     = value;
        }
    }

    freeReplyObject(reply);
    LOG_INFO(
        "[RedisManager] HGETALL success: key={}, fields={}",
        key,
        result.size());

    return result;
}

std::vector<std::string> RedisManager::HKeys(const std::string& key) {
    RedisConnGuard           guard(_pool.get());
    redisContext*            context = guard.get();
    std::vector<std::string> result;

    if (!context) {
        LOG_ERROR("[RedisManager] HKEYS failed: no available connection");
        return result;
    }

    redisReply* reply
        = (redisReply*) redisCommand(context, "HKEYS %s", key.c_str());

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] HKEYS failed: command error for key: {}", key);
        return result;
    }

    if (reply->type == REDIS_REPLY_NIL) {
        LOG_DEBUG("[RedisManager] HKEYS: key={} does not exist", key);
        freeReplyObject(reply);
        return result;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        LOG_ERROR(
            "[RedisManager] HKEYS failed: wrong type: {}, expected array",
            reply->type);
        freeReplyObject(reply);
        return result;
    }

    for (size_t i = 0; i < reply->elements; i++) {
        result.push_back(reply->element[i]->str);
    }

    freeReplyObject(reply);
    LOG_INFO(
        "[RedisManager] HKEYS success: key={}, count={}", key, result.size());

    return result;
}

std::vector<std::string> RedisManager::HVals(const std::string& key) {
    RedisConnGuard           guard(_pool.get());
    redisContext*            context = guard.get();
    std::vector<std::string> result;

    if (!context) {
        LOG_ERROR("[RedisManager] HVALS failed: no available connection");
        return result;
    }

    redisReply* reply
        = (redisReply*) redisCommand(context, "HVALS %s", key.c_str());

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] HVALS failed: command error for key: {}", key);
        return result;
    }

    if (reply->type == REDIS_REPLY_NIL) {
        LOG_DEBUG("[RedisManager] HVALS: key={} does not exist", key);
        freeReplyObject(reply);
        return result;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        LOG_ERROR(
            "[RedisManager] HVALS failed: wrong type: {}, expected array",
            reply->type);
        freeReplyObject(reply);
        return result;
    }

    for (size_t i = 0; i < reply->elements; i++) {
        result.push_back(reply->element[i]->str);
    }

    freeReplyObject(reply);
    LOG_INFO(
        "[RedisManager] HVALS success: key={}, count={}", key, result.size());

    return result;
}

bool RedisManager::HMSet(
    const std::string&                        key,
    const std::map<std::string, std::string>& field_values) {

    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] HMSET failed: no available connection");
        return false;
    }

    // 构建HMSET命令
    std::string cmd = "HMSET " + key;
    for (const auto& [field, value] : field_values) {
        cmd += " " + field + " " + value;
    }

    redisReply* reply = (redisReply*) redisCommand(context, cmd.c_str());

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] HMSET failed: command error for key: {}", key);
        return false;
    }

    bool ok
        = (reply->type == REDIS_REPLY_STATUS
           && strcasecmp(reply->str, "OK") == 0);
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO(
            "[RedisManager] HMSET success: key={}, fields={}",
            key,
            field_values.size());
    } else {
        LOG_ERROR("[RedisManager] HMSET failed: key={}", key);
    }

    return ok;
}

bool RedisManager::HExists(const std::string& key, const std::string& field) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] HEXISTS failed: no available connection");
        return false;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "HEXISTS %s %s", key.c_str(), field.c_str());

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] HEXISTS failed: command error for key: {}, field: "
            "{}",
            key,
            field);
        return false;
    }

    bool exists = (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);

    LOG_DEBUG(
        "[RedisManager] HEXISTS: key={}, field={}, exists={}",
        key,
        field,
        exists);

    return exists;
}

long long RedisManager::HLen(const std::string& key) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] HLEN failed: no available connection");
        return -1;
    }

    redisReply* reply
        = (redisReply*) redisCommand(context, "HLEN %s", key.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] HLEN failed: command error for key: {}", key);
        return -1;
    }

    long long len = -1;
    if (reply->type == REDIS_REPLY_INTEGER) {
        len = reply->integer;
    }

    freeReplyObject(reply);
    LOG_DEBUG("[RedisManager] HLEN: key={}, len={}", key, len);

    return len;
}


// 过期时间相关操作

bool RedisManager::PExpire(const std::string& key, long long milliseconds) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] PEXPIRE failed: no available connection");
        return false;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "PEXPIRE %s %lld", key.c_str(), milliseconds);

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] PEXPIRE failed: command error for key: {}", key);
        return false;
    }

    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO(
            "[RedisManager] PEXPIRE success: key={}, milliseconds={}",
            key,
            milliseconds);
    } else {
        LOG_WARN("[RedisManager] PEXPIRE failed: key={} does not exist", key);
    }

    return ok;
}

long long RedisManager::TTL(const std::string& key) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] TTL failed: no available connection");
        return -1;
    }

    redisReply* reply
        = (redisReply*) redisCommand(context, "TTL %s", key.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] TTL failed: command error for key: {}", key);
        return -1;
    }

    long long ttl = -1;
    if (reply->type == REDIS_REPLY_INTEGER) {
        ttl = reply->integer;
    }

    freeReplyObject(reply);
    LOG_DEBUG("[RedisManager] TTL: key={}, ttl={}", key, ttl);

    return ttl;
}

long long RedisManager::PTTL(const std::string& key) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] PTTL failed: no available connection");
        return -1;
    }

    redisReply* reply
        = (redisReply*) redisCommand(context, "PTTL %s", key.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] PTTL failed: command error for key: {}", key);
        return -1;
    }

    long long pttl = -1;
    if (reply->type == REDIS_REPLY_INTEGER) {
        pttl = reply->integer;
    }

    freeReplyObject(reply);
    LOG_DEBUG("[RedisManager] PTTL: key={}, pttl={}", key, pttl);

    return pttl;
}

// 键重命名操作

bool RedisManager::Rename(
    const std::string& old_key, const std::string& new_key) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] RENAME failed: no available connection");
        return false;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "RENAME %s %s", old_key.c_str(), new_key.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] RENAME failed: command error");
        return false;
    }

    bool ok
        = (reply->type == REDIS_REPLY_STATUS
           && strcasecmp(reply->str, "OK") == 0);
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO("[RedisManager] RENAME success: {} -> {}", old_key, new_key);
    } else {
        LOG_ERROR("[RedisManager] RENAME failed");
    }

    return ok;
}

bool RedisManager::RenameNX(
    const std::string& old_key, const std::string& new_key) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] RENAMENX failed: no available connection");
        return false;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "RENAMENX %s %s", old_key.c_str(), new_key.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] RENAMENX failed: command error");
        return false;
    }

    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO("[RedisManager] RENAMENX success: {} -> {}", old_key, new_key);
    } else {
        LOG_WARN("[RedisManager] RENAMENX failed: target key already exists");
    }

    return ok;
}

// 批量删除操作

long long RedisManager::Del(const std::vector<std::string>& keys) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] DEL (batch) failed: no available connection");
        return 0;
    }

    if (keys.empty()) {
        return 0;
    }

    // 构建DEL命令
    std::string cmd = "DEL";
    for (const auto& key : keys) {
        cmd += " " + key;
    }

    redisReply* reply = (redisReply*) redisCommand(context, cmd.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] DEL (batch) failed: command error");
        return 0;
    }

    long long count = 0;
    if (reply->type == REDIS_REPLY_INTEGER) {
        count = reply->integer;
    }

    freeReplyObject(reply);
    LOG_INFO("[RedisManager] DEL (batch) success: deleted {} keys", count);

    return count;
}

// 字符串高级操作

bool RedisManager::SetEx(
    const std::string& key, int seconds, const std::string& value) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] SETEX failed: no available connection");
        return false;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "SETEX %s %d %s", key.c_str(), seconds, value.c_str());

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] SETEX failed: command error for key: {}", key);
        return false;
    }

    bool ok
        = (reply->type == REDIS_REPLY_STATUS
           && strcasecmp(reply->str, "OK") == 0);
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO(
            "[RedisManager] SETEX success: key={}, seconds={}", key, seconds);
    } else {
        LOG_ERROR("[RedisManager] SETEX failed: key={}", key);
    }

    return ok;
}

bool RedisManager::SetNX(const std::string& key, const std::string& value) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] SETNX failed: no available connection");
        return false;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "SETNX %s %s", key.c_str(), value.c_str());

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] SETNX failed: command error for key: {}", key);
        return false;
    }

    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO("[RedisManager] SETNX success: key={}", key);
    } else {
        LOG_DEBUG("[RedisManager] SETNX: key={} already exists", key);
    }

    return ok;
}

bool RedisManager::MSet(const std::map<std::string, std::string>& key_values) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] MSET failed: no available connection");
        return false;
    }

    if (key_values.empty()) {
        return false;
    }

    // 构建MSET命令
    std::string cmd = "MSET";
    for (const auto& [key, value] : key_values) {
        cmd += " " + key + " " + value;
    }

    redisReply* reply = (redisReply*) redisCommand(context, cmd.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] MSET failed: command error");
        return false;
    }

    bool ok
        = (reply->type == REDIS_REPLY_STATUS
           && strcasecmp(reply->str, "OK") == 0);
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO("[RedisManager] MSET success: {} keys", key_values.size());
    } else {
        LOG_ERROR("[RedisManager] MSET failed");
    }

    return ok;
}

std::vector<std::string> RedisManager::MGet(
    const std::vector<std::string>& keys) {
    RedisConnGuard           guard(_pool.get());
    redisContext*            context = guard.get();
    std::vector<std::string> result;

    if (!context) {
        LOG_ERROR("[RedisManager] MGET failed: no available connection");
        return result;
    }

    if (keys.empty()) {
        return result;
    }

    // 构建MGET命令
    std::string cmd = "MGET";
    for (const auto& key : keys) {
        cmd += " " + key;
    }

    redisReply* reply = (redisReply*) redisCommand(context, cmd.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] MGET failed: command error");
        return result;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        LOG_ERROR(
            "[RedisManager] MGET failed: wrong type: {}, expected array",
            reply->type);
        freeReplyObject(reply);
        return result;
    }

    for (size_t i = 0; i < reply->elements; i++) {
        if (reply->element[i]->type == REDIS_REPLY_STRING) {
            result.push_back(reply->element[i]->str);
        } else {
            result.push_back("");   // nil or other types return empty string
        }
    }

    freeReplyObject(reply);
    LOG_INFO("[RedisManager] MGET success: {} keys", keys.size());

    return result;
}

// 计数器操作

long long RedisManager::Incr(const std::string& key) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] INCR failed: no available connection");
        return -1;
    }

    redisReply* reply
        = (redisReply*) redisCommand(context, "INCR %s", key.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] INCR failed: command error for key: {}", key);
        return -1;
    }

    long long new_value = -1;
    if (reply->type == REDIS_REPLY_INTEGER) {
        new_value = reply->integer;
    }

    freeReplyObject(reply);
    LOG_INFO(
        "[RedisManager] INCR success: key={}, new_value={}", key, new_value);

    return new_value;
}

long long RedisManager::IncrBy(const std::string& key, long long delta) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] INCRBY failed: no available connection");
        return -1;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "INCRBY %s %lld", key.c_str(), delta);

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] INCRBY failed: command error for key: {}", key);
        return -1;
    }

    long long new_value = -1;
    if (reply->type == REDIS_REPLY_INTEGER) {
        new_value = reply->integer;
    }

    freeReplyObject(reply);
    LOG_INFO(
        "[RedisManager] INCRBY success: key={}, delta={}, new_value={}",
        key,
        delta,
        new_value);

    return new_value;
}

long long RedisManager::Decr(const std::string& key) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] DECR failed: no available connection");
        return -1;
    }

    redisReply* reply
        = (redisReply*) redisCommand(context, "DECR %s", key.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] DECR failed: command error for key: {}", key);
        return -1;
    }

    long long new_value = -1;
    if (reply->type == REDIS_REPLY_INTEGER) {
        new_value = reply->integer;
    }

    freeReplyObject(reply);
    LOG_INFO(
        "[RedisManager] DECR success: key={}, new_value={}", key, new_value);

    return new_value;
}

long long RedisManager::DecrBy(const std::string& key, long long delta) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] DECRBY failed: no available connection");
        return -1;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "DECRBY %s %lld", key.c_str(), delta);

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] DECRBY failed: command error for key: {}", key);
        return -1;
    }

    long long new_value = -1;
    if (reply->type == REDIS_REPLY_INTEGER) {
        new_value = reply->integer;
    }

    freeReplyObject(reply);
    LOG_INFO(
        "[RedisManager] DECRBY success: key={}, delta={}, new_value={}",
        key,
        delta,
        new_value);

    return new_value;
}


// 列表扩展操作

bool RedisManager::LPushX(const std::string& key, const std::string& value) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] LPUSHX failed: no available connection");
        return false;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "LPUSHX %s %s", key.c_str(), value.c_str());

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] LPUSHX failed: command error for key: {}", key);
        return false;
    }

    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO("[RedisManager] LPUSHX success: {} -> {}", key, value);
    } else {
        LOG_WARN("[RedisManager] LPUSHX failed: list {} does not exist", key);
    }

    return ok;
}

bool RedisManager::RPushX(const std::string& key, const std::string& value) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] RPUSHX failed: no available connection");
        return false;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "RPUSHX %s %s", key.c_str(), value.c_str());

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] RPUSHX failed: command error for key: {}", key);
        return false;
    }

    bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO("[RedisManager] RPUSHX success: {} -> {}", key, value);
    } else {
        LOG_WARN("[RedisManager] RPUSHX failed: list {} does not exist", key);
    }

    return ok;
}

long long RedisManager::LLen(const std::string& key) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] LLEN failed: no available connection");
        return -1;
    }

    redisReply* reply
        = (redisReply*) redisCommand(context, "LLEN %s", key.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] LLEN failed: command error for key: {}", key);
        return -1;
    }

    long long len = -1;
    if (reply->type == REDIS_REPLY_INTEGER) {
        len = reply->integer;
    }

    freeReplyObject(reply);
    LOG_DEBUG("[RedisManager] LLEN: key={}, len={}", key, len);

    return len;
}

bool RedisManager::LSet(
    const std::string& key, int index, const std::string& value) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] LSET failed: no available connection");
        return false;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "LSET %s %d %s", key.c_str(), index, value.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] LSET failed: command error for key: {}", key);
        return false;
    }

    bool ok
        = (reply->type == REDIS_REPLY_STATUS
           && strcasecmp(reply->str, "OK") == 0);
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO("[RedisManager] LSET success: key[{}]={}", key, index);
    } else {
        LOG_ERROR("[RedisManager] LSET failed: key={}, index={}", key, index);
    }

    return ok;
}

std::string RedisManager::LIndex(const std::string& key, int index) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] LINDEX failed: no available connection");
        return "";
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "LINDEX %s %d", key.c_str(), index);

    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        LOG_DEBUG(
            "[RedisManager] LINDEX: key[{}] out of range or key does not exist",
            key);
        return "";
    }

    std::string value = reply->str;
    freeReplyObject(reply);
    LOG_DEBUG("[RedisManager] LINDEX: key[{}]={}", key, index);

    return value;
}

bool RedisManager::LTrim(const std::string& key, int start, int stop) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] LTRIM failed: no available connection");
        return false;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "LTRIM %s %d %d", key.c_str(), start, stop);

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] LTRIM failed: command error for key: {}", key);
        return false;
    }

    bool ok
        = (reply->type == REDIS_REPLY_STATUS
           && strcasecmp(reply->str, "OK") == 0);
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO(
            "[RedisManager] LTRIM success: key={}, range=[{},{}]",
            key,
            start,
            stop);
    } else {
        LOG_ERROR("[RedisManager] LTRIM failed: key={}", key);
    }

    return ok;
}

std::string RedisManager::RPopLPush(
    const std::string& source, const std::string& destination) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] RPOPLPUSH failed: no available connection");
        return "";
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "RPOPLPUSH %s %s", source.c_str(), destination.c_str());

    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        LOG_DEBUG(
            "[RedisManager] RPOPLPUSH: source list is empty or does not exist");
        return "";
    }

    std::string value = reply->str;
    freeReplyObject(reply);
    LOG_INFO("[RedisManager] RPOPLPUSH success: {} -> {}", source, destination);

    return value;
}

// 有序集合操作

bool RedisManager::ZAdd(
    const std::string& key, double score, const std::string& member) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] ZADD failed: no available connection");
        return false;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "ZADD %s %f %s", key.c_str(), score, member.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] ZADD failed: command error for key: {}", key);
        return false;
    }

    bool ok = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);

    if (ok) {
        LOG_INFO(
            "[RedisManager] ZADD success: key={}, member={}, score={}",
            key,
            member,
            score);
    } else {
        LOG_ERROR("[RedisManager] ZADD failed: key={}", key);
    }

    return ok;
}

double RedisManager::ZScore(const std::string& key, const std::string& member) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] ZSCORE failed: no available connection");
        return -1.0;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "ZSCORE %s %s", key.c_str(), member.c_str());

    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        LOG_DEBUG(
            "[RedisManager] ZSCORE: member does not exist in key: {}", key);
        return -1.0;
    }

    double score = 0.0;
    if (reply->type == REDIS_REPLY_STRING) {
        score = std::atof(reply->str);
    }

    freeReplyObject(reply);
    LOG_DEBUG(
        "[RedisManager] ZSCORE: key={}, member={}, score={}",
        key,
        member,
        score);

    return score;
}

std::vector<std::string> RedisManager::ZRange(
    const std::string& key, int start, int stop) {
    RedisConnGuard           guard(_pool.get());
    redisContext*            context = guard.get();
    std::vector<std::string> result;

    if (!context) {
        LOG_ERROR("[RedisManager] ZRANGE failed: no available connection");
        return result;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "ZRANGE %s %d %d", key.c_str(), start, stop);

    if (reply == nullptr) {
        LOG_ERROR(
            "[RedisManager] ZRANGE failed: command error for key: {}", key);
        return result;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        LOG_ERROR(
            "[RedisManager] ZRANGE failed: wrong type: {}, expected array",
            reply->type);
        freeReplyObject(reply);
        return result;
    }

    for (size_t i = 0; i < reply->elements; i++) {
        if (reply->element[i]->type == REDIS_REPLY_STRING) {
            result.push_back(reply->element[i]->str);
        }
    }

    freeReplyObject(reply);
    LOG_DEBUG("[RedisManager] ZRANGE: key={}, count={}", key, result.size());

    return result;
}

long long RedisManager::ZRem(
    const std::string& key, const std::string& member) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();
    if (!context) {
        LOG_ERROR("[RedisManager] ZREM failed: no available connection");
        return 0;
    }

    redisReply* reply = (redisReply*) redisCommand(
        context, "ZREM %s %s", key.c_str(), member.c_str());

    if (reply == nullptr) {
        LOG_ERROR("[RedisManager] ZREM failed: command error for key: {}", key);
        return 0;
    }

    long long count = 0;
    if (reply->type == REDIS_REPLY_INTEGER) {
        count = reply->integer;
    }

    freeReplyObject(reply);
    LOG_INFO(
        "[RedisManager] ZREM: key={}, member={}, removed={}",
        key,
        member,
        count);

    return count;
}


bool RedisManager::Scan(
    const std::string& pattern, std::vector<std::string>& keys) {
    RedisConnGuard guard(_pool.get());
    redisContext*  context = guard.get();

    if (!context) {
        return false;
    }

    std::string cursor = "0";
    keys.clear();

    do {
        redisReply* reply = (redisReply*) redisCommand(
            context,
            "SCAN %s MATCH %s COUNT 100",
            cursor.c_str(),
            pattern.c_str());

        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            if (reply) freeReplyObject(reply);
            return false;
        }

        cursor = reply->element[0]->str;

        redisReply* keys_reply = reply->element[1];
        for (size_t i = 0; i < keys_reply->elements; ++i) {
            keys.push_back(keys_reply->element[i]->str);
        }

        freeReplyObject(reply);

    } while (cursor != "0");

    return true;
}
