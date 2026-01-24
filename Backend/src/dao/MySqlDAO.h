// MySqlDAO.h
#ifndef MYSQLDAO_H_
#define MYSQLDAO_H_

#include "common/const.h"
#include "common/result.h"
#include "infra/ConfigManager.h"
#include "infra/LogManager.h"
#include "infra/MySqlPool.h"
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <functional>
#include <memory>

// RAII 守卫保持不变
class MySqlConnGuard {
public:
    MySqlConnGuard(MySqlPool* pool) : _pool(pool) {
        _connection = _pool->getConnection();
    }
    ~MySqlConnGuard() {
        if (_connection) {
            _pool->returnConnection(std::move(_connection));
        }
    }
    sql::Connection* get() { return _connection.get(); }

private:
    MySqlPool*                       _pool;
    std::unique_ptr<sql::Connection> _connection;
};

// --- 基类 DAO ---
class MySqlDAO {
public:
    MySqlDAO() {
        auto        globalConfig = ConfigManager::getInstance();
        const auto& host         = (*globalConfig)["MySQL"]["host"];
        const auto& port         = (*globalConfig)["MySQL"]["port"];
        const auto& user         = (*globalConfig)["MySQL"]["user"];
        const auto& passwd       = (*globalConfig)["MySQL"]["passwd"];
        const auto& schema       = (*globalConfig)["MySQL"]["schema"];

        // 格式化为 tcp://127.0.0.1:3306
        std::string url = "tcp://" + host + ":" + port;

        _pool.reset(new MySqlPool(url, user, passwd, schema, 5));
        LOG_INFO("[MySQL] Pool initialized for URL: {}", url);
    }
    virtual ~MySqlDAO() = default;

protected:
    // 通用执行接口：处理存储过程或复杂逻辑
    // 回调函数接收一个原生的 sql::Connection*，由基类负责生命周期
    template<typename T>
    Result<T>
    executeWithConn(std::function<Result<T>(sql::Connection*)> handler) const {
        try {
            MySqlConnGuard   guard(_pool.get());
            sql::Connection* conn = guard.get();
            if (!conn) {
                LOG_ERROR("[MySQL] Failed to get connection from pool.");
                return Result<T>::Error(
                    ErrorCodes::MYSQL_CONNECTION_ERROR);   // 约定 -1 为错误
            }
            return handler(conn);
        } catch (sql::SQLException& e) {
            LOG_ERROR(
                "[MySQL] SQLException: {} Error Code: {}, SQLState: {}",
                e.what(),
                e.getErrorCode(),
                e.getSQLState());
            return Result<T>::Error(ErrorCodes::SQL_ERROR);
        } catch (std::exception& e) {
            LOG_ERROR("[MySQL] Standard Exception: {}", e.what());
            return Result<T>::Error(ErrorCodes::SQL_STAND_EXCEPTION);
        } catch (...) {
            LOG_ERROR("[MySQL] Unknown exception occurred");
            return Result<T>::Error(ErrorCodes::MYSQL_UNKNOWN_ERROR);
        }
    }

    Result<void>
    executeWithConn(std::function<Result<void>(sql::Connection*)> handler) const {
        try {
            MySqlConnGuard   guard(_pool.get());
            sql::Connection* conn = guard.get();
            if (!conn) {
                LOG_ERROR("[MySQL] Failed to get connection from pool.");
                return Result<void>::Error(
                    ErrorCodes::MYSQL_CONNECTION_ERROR);   // 约定 -1 为错误
            }
            return handler(conn);
        } catch (sql::SQLException& e) {
            LOG_ERROR(
                "[MySQL] SQLException: {} Error Code: {}, SQLState: {}",
                e.what(),
                e.getErrorCode(),
                e.getSQLState());
            return Result<void>::Error(ErrorCodes::SQL_ERROR);
        } catch (std::exception& e) {
            LOG_ERROR("[MySQL] Standard Exception: {}", e.what());
            return Result<void>::Error(ErrorCodes::SQL_STAND_EXCEPTION);
        } catch (...) {
            LOG_ERROR("[MySQL] Unknown exception occurred");
            return Result<void>::Error(ErrorCodes::MYSQL_UNKNOWN_ERROR);
        }
    }


private:
    std::unique_ptr<MySqlPool> _pool;
};

#endif
