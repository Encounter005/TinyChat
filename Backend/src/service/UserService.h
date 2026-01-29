#ifndef USERSERVICE_H_
#define USERSERVICE_H_

#include <memory>
#include <string>
#include <vector>
#include "common/result.h"
#include "common/UserMessage.h"
#include "common/const.h"



class UserService {
public:
    static Result<int> Register(
        const std::string& user, const std::string& email,
        const std::string& passwd, const std::string& confirm,
        const std::string& varify_code);

    static Result<void> ResetPass(
        const std::string& name, const std::string& email,
        const std::string& new_pass, const std::string& varify_code);
    static Result<UserInfo> Login(const std::string& email);
    static Result<UserInfo> GetUserBase(int uid);
    static Result<void> AddFriendApply(const int& from, const int& to);
    static Result<void> AuthFriendApply(const int& from, const int& to);
    static Result<void> AddFriend(const int& from, const int& to, const std::string& back_name);
    static Result<std::vector<std::shared_ptr<ApplyInfo>>> GetApplyList(int uid);
    static Result<std::vector<std::shared_ptr<UserInfo>>> GetFriendList(int uid);
};


#endif   // USERSERVICE_H_
