#include "MessagePersistenceService.h"

#include "MessagePersistenceService.h"
#include "repository/MessagePersistenceRepository.h"
#include "infra/DistLock.h"
#include "infra/LogManager.h"
#include <thread>
#include <chrono>

MessagePersistenceService::MessagePersistenceService(
    boost::asio::io_context& io_context,
    int interval_seconds)
    : _timer(io_context)
    , _context(io_context)
    , _interval_seconds(interval_seconds)
    , _is_running(false) {
    
    LOG_INFO("[MessagePersistence] Service created with interval: {}s", interval_seconds);
}

MessagePersistenceService::~MessagePersistenceService() {
    Stop();
    LOG_INFO("[MessagePersistence] Service destroyed");
}

void MessagePersistenceService::Start() {
    if (_is_running.load()) {
        LOG_WARN("[MessagePersistence] Service already running");
        return;
    }
    
    _is_running.store(true);
    LOG_INFO("[MessagePersistence] Service started");
    
    // 启动定时器
    PersistMessages();
}

void MessagePersistenceService::Stop() {
    if (!_is_running.load()) {
        return;
    }
    
    _is_running.store(false);
    _timer.cancel();
    
    LOG_INFO("[MessagePersistence] Service stopped");
}

void MessagePersistenceService::ForcePersistOnce() {
    LOG_INFO("[MessagePersistence] Force persisting all cached messages");
    
    // 直接执行持久化，不使用定时器
    DoPersistMessages();
    
    // 等待完成
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void MessagePersistenceService::PersistMessages() {
    if (!_is_running.load()) {
        return;
    }
    
    // 设置定时器
    _timer.expires_after(std::chrono::seconds(_interval_seconds));
    _timer.async_wait([this](const boost::system::error_code& ec) {
        if (!ec && _is_running.load()) {
            DoPersistMessages();
            
            // 重新调度下一次执行
            PersistMessages();
        }
    });
}

void MessagePersistenceService::DoPersistMessages() {
    // 使用分布式锁防止多实例重复处理
    DistLock lock("lock:msg:persistence", 10, 5);
    
    if (!lock.isLocked()) {
        LOG_DEBUG("[MessagePersistence] Failed to acquire lock, skipping this round");
        return;
    }
    
    LOG_DEBUG("[MessagePersistence] Acquired lock, starting persistence");
    
    // 获取所有待处理的对话队列
    auto queues_res = MessagePersistenceRepository::GetAllChatQueues();
    if (!queues_res.IsOK()) {
        LOG_ERROR("[MessagePersistence] Failed to get chat queues");
        return;
    }
    
    auto queues = queues_res.Value();
    if (queues.empty()) {
        LOG_DEBUG("[MessagePersistence] No chat queues to process");
        return;
    }
    
    int total_persisted = 0;
    int total_failed = 0;
    
    // 对每个对话队列进行处理
    for (const auto& [from_uid, to_uid] : queues) {
        // 1. 从 Redis 批量获取消息（最多 BATCH_SIZE 条）
        auto msgs_res = MessagePersistenceRepository::GetMessagesFromCache(
            from_uid, to_uid, BATCH_SIZE);
        
        if (!msgs_res.IsOK()) {
            LOG_ERROR("[MessagePersistence] Failed to get messages: {} -> {}", from_uid, to_uid);
            total_failed++;
            continue;
        }
        
        auto messages = msgs_res.Value();
        if (messages.empty()) {
            continue;
        }
        
        // 2. 确定目标表
        std::string table_name = MessagePersistenceRepository::GetChatMessageTableName(
            from_uid, to_uid);
        
        // 3. 批量插入到 MySQL
        auto insert_res = MessagePersistenceRepository::BatchInsertToMySQL(
            table_name, messages);
        
        if (insert_res.IsOK()) {
            // 4. 插入成功，从 Redis 删除已处理的消息
            auto remove_res = MessagePersistenceRepository::RemovePersistedMessages(
                from_uid, to_uid, messages.size());
            
            if (remove_res.IsOK()) {
                total_persisted += messages.size();
                LOG_DEBUG("[MessagePersistence] Persisted {} messages: {} -> {} to {}",
                         messages.size(), from_uid, to_uid, table_name);
            } else {
                LOG_WARN("[MessagePersistence] Failed to remove cached messages: {} -> {}",
                         from_uid, to_uid);
            }
        } else {
            // 插入失败，消息保留在 Redis 中等待下次重试
            total_failed++;
            LOG_ERROR("[MessagePersistence] Failed to insert messages to {}: {} -> {}",
                     table_name, from_uid, to_uid);
        }
    }
    
    if (total_persisted > 0 || total_failed > 0) {
        LOG_INFO("[MessagePersistence] Persistence complete: {} persisted, {} failed",
                 total_persisted, total_failed);
    }
}
