#ifndef USERDAO_H_
#define USERDAO_H_

#include "MySqlDAO.h"
#include "common/singleton.h"
#include "infra/LogManager.h"
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <memory>

class UserDAO : public MySqlDAO, public SingleTon<UserDAO> {
    friend class SingleTon<UserDAO>;

public:
    int RegUser(
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
                return result;
            }

            return -1;
        });
    }

    bool CheckUserEmail(const std::string& name, const std::string& email) {
        return executeWithConn<bool>([&](sql::Connection* conn) {
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
                "SELECT email FROM user WHERE name = ?"));
            stmt->setString(1, name);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            while (res->next()) {
                auto email = res->getString("email");
                LOG_INFO("[MySQL] Check email: {}", email.asStdString());
                return email == res->getString("email");
            }
            return false;
        });
    }

    bool ResetPwd(const std::string& name, const std::string& new_pass) {
        return executeWithConn<bool>([&](sql::Connection* conn) {
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
                "UPDATE user SET pwd = ? WHERE name = ?"));
            stmt->setString(1, new_pass);
            stmt->setString(2, name);
            auto res = stmt->executeUpdate();
            LOG_INFO(
                "[MySQL]: update user: {}'s password to {}", name, new_pass);
            return res;
        });
    }

private:
    UserDAO() = default;
};

#endif   // USERDAO_H_
