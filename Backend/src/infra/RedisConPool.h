#ifndef REDISCONPOOL_H_
#define REDISCONPOOL_H_
#include <atomic>
#include <condition_variable>
#include "common/const.h"
#include <queue>
#include <mutex>
#include <hiredis/hiredis.h>
#include <iostream>

class RedisConPool {
public:
        RedisConPool(const char* host, int port, const char* passwd, std::size_t size) : _host(host), _port(port), _poolSize(size), _stop(false) {
            for(size_t i = 0; i < size; ++i) {
                auto* context = redisConnect(host, port);
                if(context == nullptr || context->err != 0) {
                    if(context != nullptr) {
                        redisFree(context);
                    }
                    continue;
                }

                auto reply = (redisReply*)redisCommand(context, "AUTH %s", passwd);
                if(reply->type == REDIS_REPLY_ERROR) {
                    LOG_DEBUG("认证失败");
                    freeReplyObject(reply);
                    continue;
                }

                freeReplyObject(reply);
                LOG_INFO("认证成功");
                _connections.push(context);
            }
        }

        ~RedisConPool() {
            std::lock_guard<std::mutex> lock(_mutex);
            while(!_connections.empty()) {
                auto* context = _connections.front();
                redisFree(context);
                _connections.pop();
            }
        }

        redisContext* getConnection() {
            std::unique_lock<std::mutex> lock(_mutex);
            _cond.wait(lock, [this](){
                if(_stop) return true;
                return !_connections.empty();
            });

            if(_stop) return nullptr;

            auto* context = _connections.front();
            _connections.pop();
            return context;
        }

        void returnConnection(redisContext* context) {
            std::lock_guard<std::mutex> lock(_mutex);
            if(_stop) {
                return;
            }
            _connections.push(context);
            _cond.notify_one();
        }

        void Close() {
            _stop.store(true);
            _cond.notify_all();
        }
private:
    std::atomic<bool> _stop;
    std::condition_variable _cond;
    std::mutex _mutex;
    int _port;
    std::size_t _poolSize;
    const char* _host;
    std::queue<redisContext*> _connections;
};


#endif // REDISCONPOOL_H_
