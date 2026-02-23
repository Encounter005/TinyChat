# Login Recent Messages Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Return the most recent 50 chat messages from all friends within 7 days when user logs into ChatServer, with Redis caching for performance.

**Architecture:** Query messages from MySQL's 16 sharded `chat_messages_*` tables, cache results per friend in Redis with 2-hour TTL, and include in `ID_CHAT_LOGIN_RSP` response.

**Tech Stack:** C++17, MySQL Connector/C++, hiredis, jsoncpp, Boost.Asio

---

## Prerequisites

- Existing codebase with ChatServer, MySQL, Redis infrastructure
- Message persistence already working (messages stored in `chat_messages_0` to `chat_messages_15`)
- RedisManager with SetEx/Get/Set methods available

---

### Task 1: Add Recent Messages Query to MsgDAO

**Files:**
- Modify: `Backend/src/dao/MsgDAO.h`

**Step 1: Add query method declaration**

Add the following method to MsgDAO class in `Backend/src/dao/MsgDAO.h` after the existing `handleMessage` method:

```cpp
Result<std::vector<std::string>> getRecentMessages(
    int uid, int days, int limit) {
    
    return executeWithConn<std::vector<std::string>>([&](sql::Connection* conn) {
        std::vector<std::string> messages;
        std::time_t now = std::time(nullptr);
        std::time_t start_time = now - (days * 24 * 60 * 60);
        
        std::vector<std::string> all_messages;
        
        for (int table_id = 0; table_id < 16; ++table_id) {
            std::string table_name = "chat_messages_" + std::to_string(table_id);
            
            std::string sql = "SELECT content, created_at FROM " + table_name + 
                " WHERE (from_uid = ? OR to_uid = ?) AND created_at >= FROM_UNIXTIME(?)"
                " ORDER BY created_at DESC LIMIT ?";
            
            try {
                std::unique_ptr<sql::PreparedStatement> stmt(
                    conn->prepareStatement(sql));
                stmt->setInt(1, uid);
                stmt->setInt(2, uid);
                stmt->setInt(3, static_cast<int>(start_time));
                stmt->setInt(4, limit);
                
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                while (res->next()) {
                    all_messages.push_back(res->getString("content"));
                }
            } catch (sql::SQLException& e) {
                LOG_DEBUG("Table {} query skipped: {}", table_name, e.what());
            }
        }
        
        std::sort(all_messages.begin(), all_messages.end(), 
            [](const std::string& a, const std::string& b) {
                Json::Reader reader;
                Json::Value a_root, b_root;
                if (!reader.parse(a, a_root) || !reader.parse(b, b_root)) {
                    return false;
                }
                return true;
            });
        
        size_t result_count = std::min(all_messages.size(), static_cast<size_t>(limit));
        for (size_t i = 0; i < result_count; ++i) {
            messages.push_back(all_messages[i]);
        }
        
        LOG_INFO("Retrieved {} recent messages for uid {}", messages.size(), uid);
        return Result<std::vector<std::string>>::OK(messages);
    });
}
```

**Step 2: Verify compilation**

Run: `cd Backend/build && make -j$(nproc)`
Expected: Compilation succeeds without errors

**Step 3: Commit**

```bash
git add Backend/src/dao/MsgDAO.h
git commit -m "feat(dao): add getRecentMessages query method"
```

---

### Task 2: Add Cache Logic to MessagePersistenceRepository

**Files:**
- Modify: `Backend/src/repository/MessagePersistenceRepository.h`
- Modify: `Backend/src/repository/MessagePersistenceRepository.cpp`

**Step 1: Add cache constants to header**

In `Backend/src/repository/MessagePersistenceRepository.h`, add after line 19 (after `CHAT_META_PREFIX`):

```cpp
static const std::string RECENT_MSG_PREFIX;
static const int CACHE_TTL_SECONDS = 7200;  // 2 hours
```

**Step 2: Add method declarations to header**

In `Backend/src/repository/MessagePersistenceRepository.h`, add after `GetChatMessageTableName`:

```cpp
static Result<std::vector<std::string>> GetRecentMessagesWithCache(
    int uid, int days, int limit);
static Result<void> CacheRecentMessages(
    int uid, int friend_uid, const std::string& messages_json);
static Result<std::string> GetCachedRecentMessages(
    int uid, int friend_uid);
```

**Step 3: Add constant definition in cpp**

In `Backend/src/repository/MessagePersistenceRepository.cpp`, add after line 6:

```cpp
const std::string MessagePersistenceRepository::RECENT_MSG_PREFIX = "recent:msgs:";
```

**Step 4: Implement GetCachedRecentMessages**

In `Backend/src/repository/MessagePersistenceRepository.cpp`, add at the end:

```cpp
Result<std::string> MessagePersistenceRepository::GetCachedRecentMessages(
    int uid, int friend_uid) {
    auto redis = RedisManager::getInstance();
    std::string cache_key = RECENT_MSG_PREFIX + std::to_string(uid) + ":" 
                           + std::to_string(friend_uid);
    
    std::string cached;
    if (redis->Get(cache_key, cached)) {
        LOG_DEBUG("Cache hit for recent messages: {} -> {}", uid, friend_uid);
        return Result<std::string>::OK(cached);
    }
    
    LOG_DEBUG("Cache miss for recent messages: {} -> {}", uid, friend_uid);
    return Result<std::string>::Error(ErrorCodes::REDIS_ERROR);
}
```

**Step 5: Implement CacheRecentMessages**

Add after GetCachedRecentMessages:

```cpp
Result<void> MessagePersistenceRepository::CacheRecentMessages(
    int uid, int friend_uid, const std::string& messages_json) {
    auto redis = RedisManager::getInstance();
    std::string cache_key = RECENT_MSG_PREFIX + std::to_string(uid) + ":" 
                           + std::to_string(friend_uid);
    
    if (!redis->SetEx(cache_key, CACHE_TTL_SECONDS, messages_json)) {
        LOG_WARN("Failed to cache recent messages: {} -> {}", uid, friend_uid);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }
    
    LOG_DEBUG("Cached recent messages: {} -> {} (TTL: {}s)", 
              uid, friend_uid, CACHE_TTL_SECONDS);
    return Result<void>::OK();
}
```

**Step 6: Implement GetRecentMessagesWithCache**

Add after CacheRecentMessages:

```cpp
Result<std::vector<std::string>> MessagePersistenceRepository::GetRecentMessagesWithCache(
    int uid, int days, int limit) {
    
    auto friend_list_res = UserService::GetFriendList(uid);
    if (!friend_list_res.IsOK()) {
        LOG_WARN("Failed to get friend list for uid {}", uid);
        return Result<std::vector<std::string>>::Error(friend_list_res.Error());
    }
    
    auto friends = friend_list_res.Value();
    std::vector<std::string> all_messages;
    std::vector<std::pair<int, int>> cache_miss_friends;
    
    for (const auto& friend_info : friends) {
        auto cached_res = GetCachedRecentMessages(uid, friend_info->uid);
        if (cached_res.IsOK()) {
            Json::Reader reader;
            Json::Value cached_root;
            if (reader.parse(cached_res.Value(), cached_root) && cached_root.isArray()) {
                for (const auto& msg : cached_root) {
                    all_messages.push_back(msg.asString());
                }
            }
        } else {
            cache_miss_friends.push_back({uid, friend_info->uid});
        }
    }
    
    if (!cache_miss_friends.empty()) {
        auto db_res = MsgDAO::getInstance()->getRecentMessages(uid, days, limit);
        if (db_res.IsOK()) {
            auto db_messages = db_res.Value();
            for (const auto& msg : db_messages) {
                all_messages.push_back(msg);
            }
        }
    }
    
    std::vector<std::string> result;
    size_t result_count = std::min(all_messages.size(), static_cast<size_t>(limit));
    for (size_t i = 0; i < result_count; ++i) {
        result.push_back(all_messages[i]);
    }
    
    LOG_INFO("Retrieved {} recent messages for uid {} (cache hits: {}, misses: {})",
             result.size(), uid, friends.size() - cache_miss_friends.size(), cache_miss_friends.size());
    
    return Result<std::vector<std::string>>::OK(result);
}
```

**Step 7: Add necessary includes**

At the top of `Backend/src/repository/MessagePersistenceRepository.cpp`, add:

```cpp
#include "service/UserService.h"
#include <algorithm>
```

**Step 8: Verify compilation**

Run: `cd Backend/build && make -j$(nproc)`
Expected: Compilation succeeds

**Step 9: Commit**

```bash
git add Backend/src/repository/MessagePersistenceRepository.h Backend/src/repository/MessagePersistenceRepository.cpp
git commit -m "feat(repo): add recent messages cache with Redis"
```

---

### Task 3: Update LogicHandler to Include Recent Messages in Login Response

**Files:**
- Modify: `Backend/servers/ChatServer/LogicHandler.cpp`

**Step 1: Add include for MessagePersistenceRepository**

At the top of `Backend/servers/ChatServer/LogicHandler.cpp`, the include already exists. Verify line 15 has:
```cpp
#include "repository/MessagePersistenceRepository.h"
```

**Step 2: Add recent messages to HandleLogin response**

In `Backend/servers/ChatServer/LogicHandler.cpp`, find the section after friend_list is populated (around line 151), and add before the session binding (around line 156):

```cpp
    // Get recent messages for the user
    auto recent_msgs_res = MessagePersistenceRepository::GetRecentMessagesWithCache(uid, 7, 50);
    if (recent_msgs_res.IsOK()) {
        auto recent_messages = recent_msgs_res.Value();
        if (!recent_messages.empty()) {
            for (const auto& msg_json : recent_messages) {
                root["recent_messages"].append(msg_json);
            }
            LOG_DEBUG("Added {} recent messages to login response for uid {}", 
                      recent_messages.size(), uid);
        }
    } else {
        LOG_WARN("Failed to get recent messages for uid {}, continuing login", uid);
    }
```

**Step 3: Verify compilation**

Run: `cd Backend/build && make -j$(nproc)`
Expected: Compilation succeeds

**Step 4: Commit**

```bash
git add Backend/servers/ChatServer/LogicHandler.cpp
git commit -m "feat(chatserver): include recent messages in login response"
```

---

### Task 4: Update AGENT.md Documentation

**Files:**
- Modify: `AGENT.md`

**Step 1: Update ID_CHAT_LOGIN_RSP message format**

In `AGENT.md`, find the `ID_CHAT_LOGIN_RSP (106)` section (around line 304-317) and update the response format:

```json
{
    "error": 0,
    "uid": 1001,
    "name": "用户名",
    "nick": "昵称",
    "icon": "头像路径",
    "sex": 1,
    "token": "token",
    "apply_list": [...],
    "friend_list": [...],
    "recent_messages": [
        "{\"fromuid\":1001,\"touid\":1002,\"text_array\":[{\"msgid\":\"uuid\",\"content\":\"消息内容\"}]}",
        ...
    ]
}
```

**Step 2: Add note about recent messages**

Add after the message format:

```
**recent_messages**: 最近7天内所有好友的最近50条消息，时间倒序排列。每条消息为原始JSON字符串。
```

**Step 3: Commit**

```bash
git add AGENT.md
git commit -m "docs: update login response format with recent messages"
```

---

### Task 5: Manual Integration Testing

**Files:**
- None (manual testing)

**Step 1: Build and restart ChatServer**

Run: `cd Backend/build && make -j$(nproc) && pkill -9 ChatServer && ./servers/ChatServer/ChatServer &`
Expected: ChatServer starts successfully

**Step 2: Test login with existing user**

Use the Qt client or a TCP client to:
1. Login via GateServer (HTTP POST to `/user_login`)
2. Connect to ChatServer with the returned token
3. Send `ID_CHAT_LOGIN_REQ` (105)

**Step 3: Verify response contains recent_messages**

Check the `ID_CHAT_LOGIN_RSP` response:
- Verify `recent_messages` field exists
- Verify it contains message JSON strings
- Verify messages are sorted by time (newest first)
- Verify at most 50 messages are returned

**Step 4: Verify Redis caching**

Run: `redis-cli -h 127.0.0.1 -p 6379 -a cxy`
Execute: `KEYS recent:msgs:*`
Expected: Keys like `recent:msgs:1002:1019` exist with 2-hour TTL

**Step 5: Verify cache TTL**

Execute: `TTL recent:msgs:1002:1019`
Expected: Value between 0 and 7200 seconds

---

### Task 6: Database Schema Verification (Optional)

**Files:**
- None (database check)

**Step 1: Verify message tables have created_at column**

Run MySQL query:
```sql
SHOW COLUMNS FROM chat_messages_0 LIKE 'created_at';
```

If `created_at` column does not exist, add migration:

```sql
ALTER TABLE chat_messages_0 ADD COLUMN created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP;
-- Repeat for chat_messages_1 through chat_messages_15
```

**Step 2: Commit migration if needed**

```bash
git add Backend/sql/add_created_at_column.sql
git commit -m "feat(sql): add created_at column to message tables"
```

---

## Summary

This implementation adds:
1. **MsgDAO::getRecentMessages** - Queries 16 sharded tables for recent messages
2. **MessagePersistenceRepository cache methods** - Redis caching with 2-hour TTL per friend
3. **LogicHandler::HandleLogin update** - Includes recent messages in login response
4. **Documentation update** - AGENT.md reflects new response format

## Performance Considerations

- Cache hit: O(1) Redis lookup per friend
- Cache miss: O(n) MySQL query across 16 tables
- Memory: ~1KB per cached friend conversation (50 messages)
- TTL: 2 hours balances freshness and cache efficiency

## Future Improvements (YAGNI - Not Now)

- Background pre-warming cache for active users
- Compression for large message payloads
- Per-message pagination instead of bulk load
