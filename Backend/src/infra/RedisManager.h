#ifndef REDISMANAGER_H_
#define REDISMANAGER_H_

#include "RedisConPool.h"
#include "common/singleton.h"
#include <hiredis/hiredis.h>
#include <memory>
#include <spdlog/spdlog.h>


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
        redisContext* get() {
            return _context;
        }

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
    bool        ExistsKey(const std::string& key);
    std::string HGet(const std::string& key, const std::string& hkey);
    void        Close();

private:
    RedisManager();
    std::unique_ptr<RedisConPool> _pool;
};


#endif   // REDISMANAGER_H_
