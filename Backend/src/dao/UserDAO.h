#ifndef USERDAO_H_
#define USERDAO_H_

#include "MySqlDAO.h"
#include "common/const.h"
#include "common/result.h"
#include "common/singleton.h"
#include "infra/LogManager.h"
#include "service/UserService.h"
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <memory>

class UserDAO : public MySqlDAO, public SingleTon<UserDAO> {
    friend class SingleTon<UserDAO>;

public:
    Result<int> InsertUser(
        const std::string& name, const std::string& email,
        const std::string& pwd) {
        return executeWithConn<int>([&](sql::Connection* con) {
            std::unique_ptr<sql::PreparedStatement> stmt(
                con->prepareStatement("CALL reg_user(?,?,?,@result)"));
            stmt->setString(1, name);
            stmt->setString(2, email);
            stmt->setString(3, pwd);
            stmt->execute();
            LOG_INFO(
                "[MySQL] insert a USER(name: {} email: {} pwd: {})",
                name,
                email,
                pwd);

            std::unique_ptr<sql::Statement> stmtResult(con->createStatement());
            std::unique_ptr<sql::ResultSet> res(
                stmtResult->executeQuery("SELECT @result AS result"));

            if (res->next()) {
                int result = res->getInt("result");
                LOG_INFO("[MySQL] Register Result: {}", result);
                return Result<int>::OK();
            }
            return Result<int>::Error(ErrorCodes::SQL_OPERATION_EXCEPTION);
        });
    }

    Result<std::string> FindEmailByName(const std::string& name) {
        return executeWithConn<std::string>([&](sql::Connection* conn) {
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
                "SELECT email FROM user WHERE name = ?"));
            stmt->setString(1, name);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            while (res->next()) {
                LOG_INFO(
                    "[MySQL] Check email: {}",
                    res->getString("email").asStdString());
                auto db_email = res->getString("email").asStdString();
                return Result<std::string>::OK(db_email);
            }
            return Result<std::string>::Error(
                ErrorCodes::SQL_OPERATION_EXCEPTION);
        });
    }

    Result<void> ResetPwd(
        const std::string& name, const std::string& new_pass) const {
        return executeWithConn<void>([&](sql::Connection* conn) {
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
                "UPDATE user SET pwd = ? WHERE name = ?"));
            stmt->setString(1, new_pass);
            stmt->setString(2, name);
            auto res = stmt->executeUpdate();
            LOG_INFO(
                "[MySQL]: update user: {}'s password to {}", name, new_pass);
            if (res) {
                return Result<void>::OK();
            } else {
                return Result<void>::Error(ErrorCodes::PASSWORD_RESET_ERROR);
            }
        });
    }

    Result<UserInfo> FindUserByEmail(const std::string& email) const {
        return executeWithConn<UserInfo>([&](sql::Connection* conn) {
            std::unique_ptr<sql::PreparedStatement> stmt(
                conn->prepareStatement("SELECT * FROM user WHERE email = ?"));
            stmt->setString(1, email);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

            UserInfo userInfo;
            while (res->next()) {
                auto pwd = res->getString("pwd");
                LOG_INFO(
                    "[MySQL]: select user: {}'s password: {}",
                    email,
                    pwd.asStdString());
                userInfo.email = email;
                userInfo.uid   = res->getInt("uid");
                userInfo.name  = res->getString("name").asStdString();
                userInfo.pwd   = res->getString("pwd").asStdString();
                return Result<UserInfo>::OK(userInfo);
            }
            return Result<UserInfo>::Error(ErrorCodes::MYSQL_UNKNOWN_ERROR);
        });
    }

    Result<UserInfo> FindUserByUid(int uid) const {
        return executeWithConn<UserInfo>([&](sql::Connection* conn) {
            std::unique_ptr<sql::PreparedStatement> stmt(
                conn->prepareStatement("SELECT * FROM user WHERE uid = ?"));
            LOG_DEBUG("[MySQL] uid is: {}", uid);
            stmt->setInt(1, uid);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            UserInfo                        userInfo;
            while (res->next()) {
                userInfo.email = res->getString("email");
                userInfo.uid   = res->getInt("uid");
                userInfo.name  = res->getString("name");
                LOG_INFO("[MySQL] find userinfo: uid = {}, name = {}, email = {}", userInfo.uid, userInfo.name, userInfo.email);
                return Result<UserInfo>::OK(userInfo);
            }
            return Result<UserInfo>::Error(ErrorCodes::MYSQL_UNKNOWN_ERROR);
        });
    }

private:
    UserDAO() = default;
};

#endif   // USERDAO_H_
