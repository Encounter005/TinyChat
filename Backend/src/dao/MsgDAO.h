#ifndef MESSAGEDAO_H_
#define MESSAGEDAO_H_

#include "common/result.h"
#include "common/singleton.h"
#include "dao/MySqlDAO.h"
#include "infra/LogManager.h"
#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <json/reader.h>
#include <json/writer.h>
#include <json/value.h>
#include <map>
#include <memory>
#include <string>


struct FriendMessages {
    int                      friend_uid;
    std::vector<std::string> messages;
};


class MsgDAO : public SingleTon<MsgDAO>, public MySqlDAO {
    friend class SingleTon<MsgDAO>;

public:
    Result<void> handleMessage(
        const std::string&              table_name,
        const std::vector<std::string>& messages) {

        return executeWithConn<void>([&](sql::Connection* conn) {
            if (messages.empty()) {
                return Result<void>::OK();
            }

            Json::Reader reader;
            int          success_count = 0;
            int          error_count   = 0;

            // 逐条插入消息
            for (const auto& msg_json : messages) {
                Json::Value msg_root;
                if (!reader.parse(msg_json, msg_root)) {
                    LOG_WARN("Failed to parse message JSON, skipping");
                    error_count++;
                    continue;
                }

                // 提取字段
                int from_uid = msg_root["fromuid"].asInt();
                int to_uid   = msg_root["touid"].asInt();

                std::string msgid;
                if (msg_root["text_array"].isArray()
                    && msg_root["text_array"].size() > 0) {
                    msgid = msg_root["text_array"][0]["msgid"].asString();
                } else {
                    // 如果没有 msgid，生成一个
                    msgid = "msg_" + std::to_string(std::time(nullptr)) + "_"
                            + std::to_string(rand());
                }

                // 构建单条插入 SQL（使用普通 INSERT，允许相同 msgid）
                std::string sql = "INSERT INTO " + table_name
                                  + " (msgid, from_uid, to_uid, content) "
                                    "VALUES (?, ?, ?, ?)";

                try {
                    std::unique_ptr<sql::PreparedStatement> stmt(
                        conn->prepareStatement(sql));

                    stmt->setString(1, msgid);
                    stmt->setInt(2, from_uid);
                    stmt->setInt(3, to_uid);
                    stmt->setString(4, msg_json);

                    stmt->executeUpdate();
                    success_count++;

                } catch (sql::SQLException& e) {
                    // 记录错误但继续处理下一条
                    LOG_ERROR(
                        "Failed to insert message (msgid: {}): {}",
                        msgid,
                        e.what());
                    error_count++;
                }
            }

            // 记录处理结果
            if (success_count > 0) {
                LOG_INFO(
                    "Message persistence to {}: {} inserted, {} error",
                    table_name,
                    success_count,
                    error_count);
            }

            // 只要有至少一条成功，就返回 OK
            if (success_count > 0) {
                return Result<void>::OK();
            } else {
                LOG_ERROR("All messages failed to insert to {}", table_name);
                return Result<void>::Error(ErrorCodes::SQL_ERROR);
            }
        });
    }

    Result<std::vector<FriendMessages>> getRecentMessagesGroupedByFriend(
        int uid, int days, int limit) {
        return executeWithConn<std::vector<FriendMessages>>([&](sql::Connection*
                                                                    conn) {
            std::map<int, std::vector<std::string>> friend_message_map;
            std::time_t                             now = std::time(nullptr);
            std::time_t start_time = now - (days * 24 * 60 * 60);

            struct MessageWithTime {
                std::string content;
                std::time_t timestamp;
            };
            std::map<int, std::vector<MessageWithTime>> timed_messages_map;

            for (int table_id = 0; table_id < 16; ++table_id) {
                std::string table_name
                    = "chat_messages_" + std::to_string(table_id);
                std::string sql
                    = "SELECT content, IFNULL(UNIX_TIMESTAMP(created_at), 0) "
                      "AS created_at_ts FROM "
                      + table_name + " WHERE from_uid = ? OR to_uid = ?";

                try {
                    std::unique_ptr<sql::PreparedStatement> stmt(
                        conn->prepareStatement(sql));
                    stmt->setInt(1, uid);
                    stmt->setInt(2, uid);

                    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
                    while (res->next()) {
                        std::string content = res->getString("content");
                        std::time_t db_created_at
                            = static_cast<std::time_t>(res->getInt64("created_at_ts"));

                        Json::Value  root;
                        Json::Reader reader;
                        if (!reader.parse(content, root)) {
                            LOG_WARN("Failed to parse message JSON, skipping");
                            continue;
                        }

                        int from_uid   = root["fromuid"].asInt();
                        int to_uid     = root["touid"].asInt();
                        int friend_uid = (from_uid == uid) ? to_uid : from_uid;

                        std::time_t msg_time = 0;
                        if (root.isMember("timestamp")
                            && root["timestamp"].isInt64()) {
                            msg_time
                                = static_cast<std::time_t>(root["timestamp"].asInt64());
                        }

                        if (msg_time <= 0
                            && root["text_array"].isArray()
                            && root["text_array"].size() > 0) {
                            const auto& first = root["text_array"][0];
                            if (first.isMember("timestamp")
                                && first["timestamp"].isInt64()) {
                                msg_time = static_cast<std::time_t>(
                                    first["timestamp"].asInt64());
                            }
                        }

                        if (msg_time <= 0
                            && root["text_array"].isArray()
                            && root["text_array"].size() > 0) {
                            std::string msgid
                                = root["text_array"][0]["msgid"].asString();
                            if (msgid.find("msg_") == 0) {
                                size_t second_underscore = msgid.find('_', 4);
                                if (second_underscore != std::string::npos) {
                                    std::string timestamp_str
                                        = msgid.substr(4, second_underscore - 4);
                                    try {
                                        msg_time = static_cast<std::time_t>(
                                            std::stoll(timestamp_str));
                                    } catch (...) {
                                        msg_time = 0;
                                    }
                                }
                            }
                        }
                        if (msg_time <= 0 && db_created_at > 0) {
                            msg_time = db_created_at;
                        }
                        if (msg_time <= 0) {
                            msg_time = now;
                        }

                        if (!root.isMember("timestamp") && msg_time > 0) {
                            root["timestamp"]
                                = static_cast<Json::Int64>(msg_time);
                            Json::FastWriter writer;
                            content = writer.write(root);
                        }

                        if (msg_time >= start_time && msg_time <= now) {
                            timed_messages_map[friend_uid].push_back(
                                {content, msg_time});
                        }
                    }

                } catch (sql::SQLException& e) {
                    LOG_DEBUG("Table {} query skipped: {}", table_id, e.what());
                }
            }

            std::vector<FriendMessages> result;
            for (auto& [friend_uid, messages] : timed_messages_map) {
                std::sort(
                    messages.begin(),
                    messages.end(),
                    [](const MessageWithTime& a, const MessageWithTime& b) {
                        return a.timestamp > b.timestamp;
                    });

                FriendMessages fm;
                fm.friend_uid = friend_uid;

                // limit num
                size_t count
                    = std::min(messages.size(), static_cast<size_t>(limit));
                for (size_t i = 0; i < count; ++i) {
                    fm.messages.push_back(messages[i].content);
                }

                result.push_back(fm);
            }

            return Result<std::vector<FriendMessages>>::OK(result);
        });
    }
};

#endif   // MESSAGEDAO_H_
