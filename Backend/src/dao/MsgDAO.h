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
            // 解析第一条消息获取字段
            Json::Value  first_msg;
            Json::Reader reader;
            if (!reader.parse(messages[0], first_msg)) {
                LOG_ERROR("Failed to parse message JSON");
                return Result<void>::Error(ErrorCodes::ERROR_JSON);
            }

            int from_uid = first_msg["fromuid"].asInt();
            int to_uid   = first_msg["touid"].asInt();

            // 构建批量插入 SQL
            std::ostringstream sql;
            sql << "INSERT INTO " << table_name
                << " (msgid, from_uid, to_uid, content) VALUES ";

            // 构建占位符
            for (size_t i = 0; i < messages.size(); ++i) {
                if (i > 0) sql << ", ";
                sql << "(?, ?, ?, ?)";
            }

            std::unique_ptr<sql::PreparedStatement> stmt(
                conn->prepareStatement(sql.str()));

            // 设置参数
            int param_index = 1;
            for (const auto& msg_json : messages) {
                Json::Value msg_root;
                if (!reader.parse(msg_json, msg_root)) {
                    LOG_WARN("Failed to parse message, skipping");
                    continue;
                }

                // 提取 msgid（从 text_array 中获取第一个消息的 msgid）
                if (msg_root["text_array"].isArray()
                    && msg_root["text_array"].size() > 0) {
                    std::string msgid
                        = msg_root["text_array"][0]["msgid"].asString();
                    stmt->setString(param_index++, msgid);
                } else {
                    // 如果没有 msgid，生成一个
                    std::string msgid = "msg_"
                                        + std::to_string(std::time(nullptr))
                                        + "_" + std::to_string(rand());
                    stmt->setString(param_index++, msgid);
                }

                stmt->setInt(param_index++, from_uid);
                stmt->setInt(param_index++, to_uid);
                stmt->setString(param_index++, msg_json);
            }

            // 执行插入
            try {
                stmt->executeUpdate();
                LOG_INFO(
                    "Batch inserted {} messages to {}",
                    messages.size(),
                    table_name);
                return Result<void>::OK();
            } catch (sql::SQLException& e) {
                // 检查是否是重复键错误
                if (e.getErrorCode() == 1062) {
                    LOG_WARN(
                        "Duplicate msgid found, some messages may already "
                        "exist");
                    return Result<void>::OK();   // 幂等性，视为成功
                }
                LOG_ERROR("MySQL insert failed: {}", e.what());
                return Result<void>::Error(ErrorCodes::SQL_ERROR);
            }
        });
    }
};

#endif   // MESSAGEDAO_H_
