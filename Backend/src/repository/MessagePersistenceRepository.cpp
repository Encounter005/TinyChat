#include "MessagePersistenceRepository.h"
#include "dao/MsgDAO.h"
#include "infra/RedisManager.h"
#include <string>
const std::string MessagePersistenceRepository::CHAT_MSG_PREFIX  = "chat:msg:";
const std::string MessagePersistenceRepository::CHAT_META_PREFIX = "chat:meta:";


Result<void> MessagePersistenceRepository::SaveChatMessage(
    int from_uid, int to_uid, const std::string& msg_json) {
    auto        redis   = RedisManager::getInstance();
    std::string msg_key = CHAT_MSG_PREFIX + std::to_string(from_uid) + ":"
                          + std::to_string(to_uid);
    std::string meta_key = CHAT_META_PREFIX + std::to_string(from_uid) + ":"
                           + std::to_string(to_uid);

    if (!redis->LPush(msg_key, msg_json)) {
        LOG_ERROR(
            "Failed to push message to Redis cache: {}:{}", from_uid, to_uid);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    // 更新元数据（消息计数）
    redis->HIncrBy(meta_key, "count", 1);
    redis->HSet(meta_key, "last_write", std::to_string(std::time(nullptr)));

    LOG_DEBUG("Saved message to cache: {} -> {}", from_uid, to_uid);
    return Result<void>::OK();
}

Result<std::vector<std::string>>
MessagePersistenceRepository::GetMessagesFromCache(
    int from_uid, int to_uid, int count) {
    auto        redis   = RedisManager::getInstance();
    std::string msg_key = CHAT_MSG_PREFIX + std::to_string(from_uid) + ":"
                          + std::to_string(to_uid);

    // 从 Redis 读取所有消息
    std::vector<std::string> all_msgs;
    if (!redis->LRange(msg_key, 0, -1, all_msgs)) {
        LOG_ERROR("Failed to get messages from Redis: {}:{}", from_uid, to_uid);
        return Result<std::vector<std::string>>::Error(ErrorCodes::REDIS_ERROR);
    }

    if (all_msgs.empty()) {
        return Result<std::vector<std::string>>::OK(std::vector<std::string>());
    }

    // 取最后 count 条（最旧的消息）
    // List 结构：左侧是最新消息，右侧是最旧消息
    std::vector<std::string> result;
    size_t start = (all_msgs.size() > static_cast<size_t>(count))
                       ? (all_msgs.size() - count)
                       : 0;

    for (size_t i = start; i < all_msgs.size(); ++i) {
        result.push_back(all_msgs[i]);
    }

    LOG_DEBUG(
        "Retrieved {} messages from cache: {} -> {}",
        result.size(),
        from_uid,
        to_uid);
    return Result<std::vector<std::string>>::OK(result);
}

Result<void> MessagePersistenceRepository::RemovePersistedMessages(
    int from_uid, int to_uid, int count) {
    auto        redis   = RedisManager::getInstance();
    std::string msg_key = CHAT_MSG_PREFIX + std::to_string(from_uid) + ":"
                          + std::to_string(to_uid);
    std::string meta_key = CHAT_META_PREFIX + std::to_string(from_uid) + ":"
                           + std::to_string(to_uid);

    // 获取当前队列长度
    std::vector<std::string> all_msgs;
    if (!redis->LRange(msg_key, 0, -1, all_msgs)) {
        LOG_ERROR("Failed to get queue length: {}:{}", from_uid, to_uid);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    if (all_msgs.empty()) {
        return Result<void>::OK();
    }

    // 计算保留的起始位置
    long long keep_count = static_cast<long long>(all_msgs.size()) - count;
    if (keep_count <= 0) {
        // 删除全部
        redis->Del(msg_key);
        redis->Del(meta_key);
    } else {
        // 保留前面的 keep_count 条，删除后面的 count 条
        // LTRIM 保留索引 0 到 keep_count-1 的元素
        redis->LTrim(msg_key, 0, keep_count - 1);

        // 更新元数据
        redis->HSet(meta_key, "count", std::to_string(keep_count));
    }

    LOG_DEBUG(
        "Removed {} persisted messages: {} -> {}", count, from_uid, to_uid);
    return Result<void>::OK();
}


Result<void> MessagePersistenceRepository::BatchInsertToMySQL(
    const std::string& table_name, const std::vector<std::string>& messages) {
    return MsgDAO::getInstance()->handleMessage(table_name, messages);
}

Result<std::vector<std::pair<int, int>>>
MessagePersistenceRepository::GetAllChatQueues() {
    auto redis = RedisManager::getInstance();

    std::vector<std::string> keys;
    std::string              pattern = CHAT_MSG_PREFIX + "*";

    if (!redis->Scan(pattern, keys)) {
        LOG_ERROR("Failed to scan chat message queues");
        return Result<std::vector<std::pair<int, int>>>::Error(
            ErrorCodes::REDIS_ERROR);
    }

    std::vector<std::pair<int, int>> result;
    for (const auto& key : keys) {
        // 解析 key: "chat:msg:from:to" -> (from, to)
        std::string prefix = CHAT_MSG_PREFIX;
        if (key.substr(0, prefix.length()) == prefix) {
            std::string rest      = key.substr(prefix.length());
            size_t      colon_pos = rest.find(':');
            if (colon_pos != std::string::npos) {
                try {
                    int from_uid = std::stoi(rest.substr(0, colon_pos));
                    int to_uid   = std::stoi(rest.substr(colon_pos + 1));
                    result.push_back({from_uid, to_uid});
                } catch (const std::exception& e) {
                    LOG_WARN("Failed to parse key: {}", key);
                }
            }
        }
    }

    LOG_DEBUG("Found {} chat queues", result.size());
    return Result<std::vector<std::pair<int, int>>>::OK(result);
}

int MessagePersistenceRepository::GetChatMessageTable(
    int from_uid, int to_uid) {
    return (from_uid + to_uid) % 16;
}

std::string MessagePersistenceRepository::GetChatMessageTableName(
    int from_uid, int to_uid) {
    return "chat_messags_"
           + std::to_string(GetChatMessageTable(from_uid, to_uid));
}
