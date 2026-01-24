#ifndef USERSERVICE_H_
#define USERSERVICE_H_

#include <string>
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
};


#endif   // USERSERVICE_H_
