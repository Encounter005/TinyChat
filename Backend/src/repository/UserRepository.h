#ifndef USERREPOSITORY_H_
#define USERREPOSITORY_H_

#include "common/singleton.h"
#include "common/result.h"
#include "common/UserMessage.h"
#include "infra/RedisManager.h"
#include <memory>
#include <string>

class UserRepository : public SingleTon<UserRepository> {
    friend class SingleTon<UserRepository>;

public:
    // Read operations with caching
    static Result<UserInfo> getUserById(int uid);
    static Result<std::string> getEmailByName(const std::string& name);

    // Read operation for authentication (bypasses cache for security)
    static Result<UserInfo> findUserAuthInfoByEmail(const std::string& email);

    // Write operations with cache invalidation
    static Result<int> createUser(
        const std::string& name, const std::string& email,
        const std::string& pwd);
    static Result<void> resetPassword(
        const std::string& name, const std::string& new_pass);

    // Direct cache manipulation
    static void clearUserCache(int uid);
    static void UserIpWithServer(const std::string& ip, const std::string& server_name);

private:
    UserRepository()  = default;
    ~UserRepository() = default;
};

#endif   // USERREPOSITORY_H_
