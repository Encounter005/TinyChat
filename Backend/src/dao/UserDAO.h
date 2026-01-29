#ifndef USERDAO_H_
#define USERDAO_H_

#include "MySqlDAO.h"
#include "common/UserMessage.h"
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
#include <vector>

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
            stmt->setInt(1, uid);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            UserInfo                        userInfo;
            while (res->next()) {
                userInfo.email = res->getString("email");
                userInfo.uid   = res->getInt("uid");
                userInfo.name  = res->getString("name");
                userInfo.icon  = res->getString("icon");
                userInfo.nick  = res->getString("nick");
                LOG_INFO(
                    "[MySQL] find userinfo: uid = {}, name = {}, email = {}",
                    userInfo.uid,
                    userInfo.name,
                    userInfo.email);
                return Result<UserInfo>::OK(userInfo);
            }
            return Result<UserInfo>::Error(ErrorCodes::MYSQL_UNKNOWN_ERROR);
        });
    }
    Result<void> AddFriendApply(const int& from, const int& to) {
        return executeWithConn<void>([&](sql::Connection* conn) {
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
                "INSERT INTO friend_apply (from_uid, to_uid) values (?,?) "
                "ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = "
                "to_uid "));
            stmt->setInt(1, from);
            stmt->setInt(2, to);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            if (res) {
                return Result<void>::OK();
            } else {
                return Result<void>::Error(ErrorCodes::APPLYFRIEND_ERROR);
            }
        });
    }
    Result<std::vector<std::shared_ptr<ApplyInfo>>> GetApplyList(
        int touid, int begin, int limit) {
        return executeWithConn<std::vector<
            std::shared_ptr<ApplyInfo>>>([&](sql::Connection* conn) {
            std::vector<std::shared_ptr<ApplyInfo>> app_list;
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
                "select apply.from_uid, apply.status, user.name, "
                "user.nick, user.sex from friend_apply as apply join user on "
                "apply.from_uid = user.uid where apply.to_uid = ? "
                "and apply.id > ? order by apply.id ASC LIMIT ? "));
            stmt->setInt(1, touid);
            stmt->setInt(2, begin);
            stmt->setInt(3, limit);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            while (res->next()) {
                auto name      = res->getString("name");
                auto uid       = res->getInt("from_uid");
                auto status    = res->getInt("status");
                auto nick      = res->getString("nick");
                auto sex       = res->getInt("sex");
                auto apply_ptr = std::make_shared<ApplyInfo>(
                    uid, name, "", "", nick, sex, status);
                app_list.push_back(apply_ptr);
            }
            return Result<std::vector<std::shared_ptr<ApplyInfo>>>::OK(
                app_list);
        });
    }


    Result<void> AuthFriendApply(const int& from, const int& to) {
        return executeWithConn<void>([&](sql::Connection* conn) {
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
                "UPDATE friend_apply SET status = 1 "
                "WHERE from_uid = ? AND to_uid = ?"));
            stmt->setInt(1, from);
            stmt->setInt(2, to);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            if (res) {
                return Result<void>::OK();
            } else {
                return Result<void>::Error(ErrorCodes::MYSQL_UNKNOWN_ERROR);
            }
        });
    }

    Result<void> AddFriend(
        const int& from, const int& to, const std::string& back_name) {
        return executeWithConn<void>([&](sql::Connection* conn) {
            std::unique_ptr<sql::PreparedStatement> stmt1(
                conn->prepareStatement(
                    "INSERT IGNORE INTO friend(self_id, friend_id, back) "
                    "VALUES (?, ?, ?) "));
            stmt1->setInt(1, from);
            stmt1->setInt(2, to);
            stmt1->setString(3, back_name);
            std::unique_ptr<sql::ResultSet> res1(stmt1->executeQuery());
            if (!res1) {
                return Result<void>::Error(ErrorCodes::MYSQL_UNKNOWN_ERROR);
            }

            std::unique_ptr<sql::PreparedStatement> stmt2(
                conn->prepareStatement(
                    "INSERT IGNORE INTO friend(self_id, friend_id, back) "
                    "VALUES (?, ?, ?) "));
            stmt2->setInt(1, to);
            stmt2->setInt(2, from);
            stmt2->setString(3, "");
            std::unique_ptr<sql::ResultSet> res2(stmt2->executeQuery());
            if (!res2) {
                return Result<void>::Error(ErrorCodes::MYSQL_UNKNOWN_ERROR);
            }

            return Result<void>::OK();
        });
    }

    using ArrayUserInfo = std::vector<std::shared_ptr<UserInfo>>;
    Result<ArrayUserInfo> GetFriendList(int uid) {
        return executeWithConn<ArrayUserInfo>([&](sql::Connection* conn) {
            ArrayUserInfo                           user_info_list;
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
                "select user.uid, user.name, user.email, user.nick, "
                "user.desc, user.sex, user.icon,  friend.back as "
                "remark from friend join user on friend.friend_id = user.uid "
                "where friend.self_id = ? "));
            stmt->setInt(1, uid);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            while (res->next()) {
                auto user_info   = std::make_shared<UserInfo>();
                user_info->uid   = res->getInt("uid");
                user_info->name  = res->getString("name");
                user_info->email = res->getString("email");
                user_info->nick  = res->getString("nick");
                user_info->desc  = res->getString("desc");
                user_info->sex   = res->getInt("sex");
                user_info->icon  = res->getString("icon");
                user_info_list.push_back(user_info);
            }
            return Result<ArrayUserInfo>::OK(user_info_list);
        });
    }

private:
    UserDAO() = default;
};

#endif   // USERDAO_H_
