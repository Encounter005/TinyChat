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
};

#endif   // MESSAGEDAO_H_
