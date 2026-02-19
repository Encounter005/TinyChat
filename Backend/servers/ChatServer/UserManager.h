#ifndef USERMANAGER_H_
#define USERMANAGER_H_

#include "common/singleton.h"
#include "session.h"
#include <unordered_map>
#include <shared_mutex>

class UserManager : public SingleTon<UserManager> {
    friend class SingleTon<UserManager>;
public:
        void Bind(int uid, std::shared_ptr<Session> session);
        void UnBind(int uid);
        std::shared_ptr<Session> GetSession(int uid);
        void KickUser(int uid, const std::string& reason);

private:
        UserManager() = default;
        std::shared_mutex _mtx;
        std::unordered_map<int, std::shared_ptr<Session>> _uid_to_session;
};


#endif // USERMANAGER_H_
