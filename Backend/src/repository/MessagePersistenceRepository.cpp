#include "MessagePersistenceRepository.h"
#include "common/const.h"
#include "common/result.h"
#include "dao/MsgDAO.h"
#include "infra/LogManager.h"
#include "infra/RedisManager.h"
#include "service/UserService.h"
#include <json/reader.h>
#include <json/writer.h>
#include <string>
#include <algorithm>
#include <vector>
const std::string MessagePersistenceRepository::CHAT_MSG_PREFIX  = "chat:msg:";
const std::string MessagePersistenceRepository::CHAT_META_PREFIX = "chat:meta:";
const std::string MessagePersistenceRepository::RECENT_MSG_PREFIX
    = "recent:msgs:";
const int MessagePersistenceRepository::CACHE_TTL_SECONDS = 7200;   // 2 hours

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
    return "chat_messages_"
           + std::to_string(GetChatMessageTable(from_uid, to_uid));
}


Result<std::vector<FriendMessages>>
MessagePersistenceRepository::GetRecentMessagesWithCache(
    int uid, int days, int limit) {
    auto friend_list_res = UserService::GetFriendList(uid);
    if (!friend_list_res.IsOK()) {
        LOG_WARN("Failed to get friend list for uid");
        return Result<std::vector<FriendMessages>>::Error(
            friend_list_res.Error());
    }

    auto                        friend_list = friend_list_res.Value();
    std::vector<FriendMessages> result;
    std::vector<int>            cache_miss_friends;

    auto extract_ts = [](const std::string& msg_json) -> int64_t {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(msg_json, root) || !root.isObject()) {
            return 0;
        }
        if (root.isMember("timestamp") && root["timestamp"].isInt64()) {
            return root["timestamp"].asInt64();
        }
        if (root.isMember("text_array") && root["text_array"].isArray()
            && root["text_array"].size() > 0) {
            const auto& first = root["text_array"][0];
            if (first.isMember("timestamp") && first["timestamp"].isInt64()) {
                return first["timestamp"].asInt64();
            }
        }
        return 0;
    };

    // get friend_info from cache
    for (const auto& friend_info : friend_list) {
        int  friend_uid = friend_info->uid;
        auto cached_res = GetCachedFriendMessages(uid, friend_uid);

        if (cached_res.IsOK()) {
            FriendMessages fm{friend_uid, cached_res.Value()};
            result.push_back(fm);
        } else {
            cache_miss_friends.push_back(friend_uid);
        }
    }

    // query for database
    if (!cache_miss_friends.empty()) {
        LOG_DEBUG(
            "Cache miss for {} friends, querying database",
            cache_miss_friends.size());

        auto db_res = MsgDAO::getInstance()->getRecentMessagesGroupedByFriend(
            uid, days, limit);

        if (db_res.IsOK()) {
            auto db_friend_messages = db_res.Value();

            for (const auto& db_fm : db_friend_messages) {
                bool exists = false;
                for (auto& existing_fm : result) {
                    if (existing_fm.friend_uid == db_fm.friend_uid) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    result.push_back(db_fm);
                    CacheFriendMessages(uid, db_fm.friend_uid, db_fm.messages);
                }
            }
        }
    }

    // BOT 会话不在好友列表中，这里补充从 Redis 缓存读取最近消息
    std::vector<std::string> bot_msgs;
    auto                     bot_in_res = GetMessagesFromCache(BOT_UID, uid, limit);
    if (bot_in_res.IsOK()) {
        auto in_msgs = bot_in_res.Value();
        bot_msgs.insert(bot_msgs.end(), in_msgs.begin(), in_msgs.end());
    }
    auto bot_out_res = GetMessagesFromCache(uid, BOT_UID, limit);
    if (bot_out_res.IsOK()) {
        auto out_msgs = bot_out_res.Value();
        bot_msgs.insert(bot_msgs.end(), out_msgs.begin(), out_msgs.end());
    }

    if (!bot_msgs.empty()) {
        std::sort(
            bot_msgs.begin(),
            bot_msgs.end(),
            [&](const std::string& a, const std::string& b) {
                return extract_ts(a) < extract_ts(b);
            });

        const size_t keep = static_cast<size_t>(std::max(limit, 1));
        if (bot_msgs.size() > keep) {
            bot_msgs.erase(bot_msgs.begin(), bot_msgs.end() - keep);
        }

        bool exists = false;
        for (const auto& fm : result) {
            if (fm.friend_uid == BOT_UID) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            result.push_back(FriendMessages{BOT_UID, bot_msgs});
        }
    }

    LOG_INFO(
        "Retrieved recent messages for uid {} (cache hits: {}, misses: {})",
        uid,
        friend_list.size() - cache_miss_friends.size(),
        cache_miss_friends.size());

    return Result<std::vector<FriendMessages>>::OK(result);
}

Result<void> MessagePersistenceRepository::CacheFriendMessages(
    int uid, int friend_uid, const std::vector<std::string>& messages) {
    if (messages.empty()) {
        return Result<void>::OK();
    }

    Json::Value root;
    for (const auto& message : messages) {
        root.append(message);
    }

    Json::FastWriter writer;
    std::string      messages_json = writer.write(root);

    auto        redis = RedisManager::getInstance();
    std::string key   = RECENT_MSG_PREFIX + std::to_string(uid) + ":"
                      + std::to_string(friend_uid);

    if (redis->SetEx(key, CACHE_TTL_SECONDS, messages_json)) {
        LOG_WARN("Failed to cache recent messages: {} -> {}", uid, friend_uid);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    LOG_INFO(
        "Cached recent messages: {} -> {} ({} messages, TTL: {})",
        uid,
        friend_uid,
        messages.size(),
        CACHE_TTL_SECONDS);
    return Result<void>::OK();
}

Result<std::vector<std::string>>
MessagePersistenceRepository::GetCachedFriendMessages(int uid, int friend_uid) {
    auto        redis     = RedisManager::getInstance();
    std::string cache_key = RECENT_MSG_PREFIX + std::to_string(uid) + ":"
                            + std::to_string(friend_uid);
    std::string cache_json;

    if (!redis->Get(cache_key, cache_json)) {
        LOG_DEBUG("Cache miss for recent messages: {} -> {}", uid, friend_uid);
        return Result<std::vector<std::string>>::Error(ErrorCodes::REDIS_ERROR);
    }

    Json::Value  root;
    Json::Reader reader;
    if (!reader.parse(cache_json, root) || !root.isArray()) {
        LOG_WARN("Invalid cached data format for {} -> {}", uid, friend_uid);
        return Result<std::vector<std::string>>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::vector<std::string> messages;

    for (const auto& msg : root) {
        messages.push_back(msg.asString());
    }

    LOG_INFO(
        "Cache hit for recent messages: {} -> {} ({} messages)",
        uid,
        friend_uid,
        messages.size());

    return Result<std::vector<std::string>>::OK(messages);
}
