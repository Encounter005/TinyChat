#ifndef USERREPOSITORY_H_
#define USERREPOSITORY_H_

#include "common/UserMessage.h"
#include "common/result.h"
#include "common/singleton.h"
#include "infra/RedisManager.h"
#include <memory>
#include <string>

class UserRepository : public SingleTon<UserRepository> {
    friend class SingleTon<UserRepository>;

public:
    // Read operations with caching
    static Result<UserInfo>    getUserById(int uid);
    static Result<std::string> getEmailByName(const std::string& name);

    // Read operation for authentication (bypasses cache for security)
    static Result<UserInfo> findUserAuthInfoByEmail(const std::string& email);

    // Write operations with cache invalidation
    static Result<int> createUser(
        const std::string& name, const std::string& email,
        const std::string& pwd);
    static Result<void> resetPassword(
        const std::string& name, const std::string& new_pass);
    static Result<void> AddFriendApply(const int& from, const int& to);
    static Result<void> AuthFriendApply(const int& from, const int& to);
    static Result<void> AddFriend(
        const int& from, const int& to, const std::string& back_name);
    // Direct cache manipulation
    static void clearUserCache(int uid);
    static void BindUserIpWithServer(
        const int& uid, const std::string& server_name);
    static void                UnBindUserIpWithServer(const int& uid);
    static Result<std::string> FindUserIpServerByUid(const int& uid);
    static Result<std::vector<std::shared_ptr<ApplyInfo>>> GetApplyList(
        int uid);
    using ArrayUserInfo = std::vector<std::shared_ptr<UserInfo>>;
    static Result<ArrayUserInfo> GetFriendList(
        int uid);

    static Result<void> SaveOfflineMessage(int uid, const std::string& msg);
    static Result<std::vector<std::string>> GetOfflineMessages(int uid);

private:
    UserRepository()  = default;
    ~UserRepository() = default;
};

#endif   // USERREPOSITORY_H_
