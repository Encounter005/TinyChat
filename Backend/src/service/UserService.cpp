#include "UserService.h"
#include "common/const.h"
#include "core/VarifyGrpcClient.h"
#include "dao/UserDAO.h"
#include "infra/LogManager.h"
#include "infra/RedisManager.h"
#include <boost/beast/core/buffers_to_string.hpp>
#include <json/reader.h>

int UserService::Register(
    const std::string &user, const std::string &email,
    const std::string &passwd, const std::string &confirm,
    const std::string &varify_code) {
    if (passwd != confirm) {
        LOG_ERROR(
            "Occur password error passwd is: {} confirm is: {}",
            passwd,
            confirm);
        return ErrorCodes::PasswdError;
    }

    std::string redis_code;
    if (!RedisManager::getInstance()->Get(email, redis_code)) {
        return ErrorCodes::VarifyExpired;
    }

    if (redis_code != varify_code) {
        return ErrorCodes::VarifyCodeError;
    }

    if (RedisManager::getInstance()->ExistsKey(user)) {
        return ErrorCodes::UserExist;
    }

    auto uid = UserDAO::getInstance()->RegUser(user, email, passwd);
    if (uid <= 0) {
        return ErrorCodes::UserExist;
    }

    return 0;
}


int UserService::ResetPass(
    const std::string &name, const std::string &email, const std::string& new_pass,
    const std::string &varify_code) {

    std::string redis_code;
    if(!RedisManager::getInstance()->Get(email, redis_code)) {
        return ErrorCodes::VarifyExpired;
    }
    if(redis_code != varify_code) {
        return ErrorCodes::VarifyCodeError;
    }

    // check if email is exists
    auto IsEmailMatch = UserDAO::getInstance()->CheckUserEmail(name, email);
    if(!IsEmailMatch) {
        return ErrorCodes::EmailNotMatch;
    }
    auto IsPasswordUpdated = UserDAO::getInstance()->ResetPwd(name, new_pass);
    if(!IsPasswordUpdated) {
        return ErrorCodes::PasswdError;
    }
    return 0;
}
