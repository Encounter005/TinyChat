#include "UserService.h"
#include "common/const.h"
#include "common/result.h"
#include "grpcClient/VarifyClient.h"
#include "infra/LogManager.h"
#include "infra/RedisManager.h"
#include "repository/UserRepository.h"
#include <boost/beast/core/buffers_to_string.hpp>
#include <json/reader.h>

Result<int> UserService::Register(
    const std::string &user, const std::string &email,
    const std::string &passwd, const std::string &confirm,
    const std::string &varify_code) {
    if (passwd != confirm) {
        LOG_ERROR(
            "Occur password error passwd is: {} confirm is: {}",
            passwd,
            confirm);
        return Result<int>::Error(ErrorCodes::PASSWORD_ERROR);
    }

    std::string redis_code;
    if (!RedisManager::getInstance()->Get(email, redis_code)) {
        return Result<int>::Error(ErrorCodes::VARIFY_EXPIRED);
    }

    if (redis_code != varify_code) {
        return Result<int>::Error(ErrorCodes::VARIFY_CODE_ERROR);
    }

    if (RedisManager::getInstance()->ExistsKey(user)) {
        return Result<int>::Error(ErrorCodes::USER_EXIST);
    }

    auto res = UserRepository::createUser(user, email, passwd);
    return res;
}


Result<void> UserService::ResetPass(
    const std::string &name, const std::string &email,
    const std::string &new_pass, const std::string &varify_code) {

    std::string redis_code;
    if (!RedisManager::getInstance()->Get(email, redis_code)) {
        return Result<void>::Error(ErrorCodes::VARIFY_EXPIRED);
    }
    if (redis_code != varify_code) {
        return Result<void>::Error(ErrorCodes::VARIFY_CODE_ERROR);
    }

    // check if email is exists
    auto ResultEmail = UserRepository::getEmailByName(name);
    if( !ResultEmail.IsOK() ) {
        return Result<void>::Error(ResultEmail.Error());
    }
    auto db_email = ResultEmail.Value();
    if(db_email != email) {
        return Result<void>::Error(ErrorCodes::EMAIL_NOT_MATCH);
    }

    auto checkPasswordResult = UserRepository::resetPassword(name, new_pass);
    if (!checkPasswordResult.IsOK()) {
        return checkPasswordResult;
    }
    return Result<void>::OK();
}

Result<UserInfo> UserService::Login(
    const std::string &email) {
    auto res = UserRepository::findUserAuthInfoByEmail(email);
    return res;
}

Result<UserInfo> UserService::GetUserBase(int uid) {
    auto res = UserRepository::getUserById(uid);
    return res;
}
