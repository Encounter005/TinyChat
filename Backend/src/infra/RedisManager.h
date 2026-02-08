#ifndef REDISMANAGER_H_
#define REDISMANAGER_H_

#include "RedisConPool.h"
#include "common/singleton.h"
#include <hiredis/hiredis.h>
#include <map>
#include <memory>
#include <spdlog/fmt/fmt.h>
#include <string>
#include <vector>


class RedisConnGuard {
public:
    RedisConnGuard(RedisConPool* pool) : _pool(pool) {
        _context = _pool->getConnection();
    }

    ~RedisConnGuard() {
        if (_context) {
            _pool->returnConnection(_context);
        }
    }
    redisContext* get() { return _context; }

private:
    RedisConPool* _pool;
    redisContext* _context;
};



class RedisManager : public SingleTon<RedisManager>,
                     std::enable_shared_from_this<RedisManager> {
    friend class SingleTon<RedisManager>;

public:
    ~RedisManager();
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool Auth(const std::string& password);
    bool LPush(const std::string& key, const std::string& value);
    bool LPop(const std::string& key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);
    bool HSet(
        const std::string& key, const std::string& hkey,
        const std::string& value);
    bool HSet(
        const char* key, const char* hkey, const char* hvalue,
        std::size_t hvalue_len);
    bool        Del(const std::string& key);
    bool        HDel(const std::string& hkey, const std::string& field);
    bool        ExistsKey(const std::string& key);
    std::string HGet(const std::string& key, const std::string& hkey);
    void        Close();
    long long   HIncrBy(
          const std::string& hkey, const std::string& field, long long delta);
    bool LRange(
        const std::string& key, int start, int stop,
        std::vector<std::string>& values);
    std::string AcquireLock(
        const std::string& lock_name, int lock_timout, int acquire_timeout);
    bool ReleaseLock(const std::string& lock_name, const std::string& id);

    // @brief: 设置键的过期时间（秒）
    bool Expire(const std::string& key, int seconds);

    // @brief: 获取哈希表中所有字段和值
    std::map<std::string, std::string> HGetAll(const std::string& key);

    // @brief: 获取哈希表中所有字段
    std::vector<std::string> HKeys(const std::string& key);

    // @brief: 获取哈希表中所有值
    std::vector<std::string> HVals(const std::string& key);

    // @brief: 批量设置哈希字段
    bool HMSet(
        const std::string&                        key,
        const std::map<std::string, std::string>& field_values);

    // @brief: 检查哈希字段是否存在
    bool HExists(const std::string& key, const std::string& field);

    // @brief: 获取哈希表中字段的数量
    long long HLen(const std::string& key);

    // @brief: 设置键的过期时间（毫秒）
    bool PExpire(const std::string& key, long long milliseconds);

    // @brief: 获取键的剩余生存时���（秒）
    long long TTL(const std::string& key);

    // @brief: 获取键的剩余生存时间（毫秒）
    long long PTTL(const std::string& key);

    // @brief: 重命名键
    bool Rename(const std::string& old_key, const std::string& new_key);

    // @brief: 仅当新键不存在时重命名
    bool RenameNX(const std::string& old_key, const std::string& new_key);

    // @brief: 批量删除键
    long long Del(const std::vector<std::string>& keys);

    // @brief: 设置键值并指定过期时间（秒）
    bool SetEx(const std::string& key, int seconds, const std::string& value);

    // @brief: 键不存在时设置值
    bool SetNX(const std::string& key, const std::string& value);

    // @brief: 同时设置多个键值对
    bool MSet(const std::map<std::string, std::string>& key_values);

    // @brief: 获取多个键的值
    std::vector<std::string> MGet(const std::vector<std::string>& keys);

    // @brief: 自增键值
    long long Incr(const std::string& key);

    // @brief: 自增指定增量
    long long IncrBy(const std::string& key, long long delta);

    // @brief: 自减键值
    long long Decr(const std::string& key);

    // @brief: 自减指定增量
    long long DecrBy(const std::string& key, long long delta);

    // @brief: 向列表添加元素（仅在列表存在时）
    bool LPushX(const std::string& key, const std::string& value);

    // @brief: 向列表尾部添加元素（仅在列表存在时）
    bool RPushX(const std::string& key, const std::string& value);

    // @brief: 获取列表长度
    long long LLen(const std::string& key);

    // @brief: 设置列表中指定索引的值
    bool LSet(const std::string& key, int index, const std::string& value);

    // @brief: 获取列表中指定索引的元素
    std::string LIndex(const std::string& key, int index);

    // @brief: 保留列表指定范围内的元素
    bool LTrim(const std::string& key, int start, int stop);

    // @brief: 从列表中弹出最后一个元素并推送到另一个列表
    std::string RPopLPush(
        const std::string& source, const std::string& destination);

    // @brief: 向有序集合添加成员
    bool ZAdd(const std::string& key, double score, const std::string& member);

    // @brief: 获取有序集合中成员的分数
    double ZScore(const std::string& key, const std::string& member);

    // @brief: 获取有序集合指定范围内的成员
    std::vector<std::string> ZRange(
        const std::string& key, int start, int stop);

    // @brief: 移除有序集合中的成员
    long long ZRem(const std::string& key, const std::string& member);

private:
    // @brief: 为每个锁分配一个uuid
    std::string generateUUID();
    RedisManager();
    std::unique_ptr<RedisConPool> _pool;
};


#endif   // REDISMANAGER_H_
