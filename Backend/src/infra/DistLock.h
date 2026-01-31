#ifndef DISTLOCK_H_
#define DISTLOCK_H_

#include <string>
#include "infra/RedisManager.h"

class DistLock {
public:
    /**
     * @brief 构造函数，在创建时尝试获取分布式锁
     * @param lock_name 锁的名称 (例如， "user_kick:12345")
     * @param lock_timeout 锁的自动过期时间（秒）。防止死锁。
     * @param acquire_timeout 获取锁的超时时间（秒）。在这段时间内会不断尝试。
     */
    DistLock(
        const std::string& lock_name, int lock_timeout = 10,
        int acquire_timeout = 5)
        : _lock_name(lock_name), _is_locked(false) {

        auto redis = RedisManager::getInstance();
        _lock_id = redis->AcquireLock(lock_name, lock_timeout, acquire_timeout);

        if (!_lock_id.empty()) {
            _is_locked = true;
        }
    }

    /**
     * @brief 析构函数，如果持有锁，则自动释放
     */
    ~DistLock() {
        if (_is_locked) {
            auto redis = RedisManager::getInstance();
            redis->ReleaseLock(_lock_name, _lock_id);
        }
    }

    /**
     * @brief 检查是否成功获取了锁
     * @return true 如果持有锁, false 如果获取失败
     */
    bool isLocked() const { return _is_locked; }

    // --- 禁止拷贝和移动，锁的上下文是唯一的 ---
    DistLock(const DistLock&)            = delete;
    DistLock& operator=(const DistLock&) = delete;
    DistLock(DistLock&&)                 = delete;
    DistLock& operator=(DistLock&&)      = delete;

private:
    std::string _lock_name;
    std::string _lock_id;
    bool        _is_locked;
};


#endif   // DISTLOCK_H_
