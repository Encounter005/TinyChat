#ifndef MESSAGEDAO_H_
#define MESSAGEDAO_H_

#include "common/result.h"
#include "common/singleton.h"
#include "dao/MySqlDAO.h"
#include <cppconn/connection.h>

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
};

#endif   // MESSAGEDAO_H_
