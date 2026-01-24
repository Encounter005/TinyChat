#include "UserManager.h"
#include <memory>
#include <mutex>
#include <shared_mutex>
#include "infra/LogManager.h"

void UserManager::Bind(int uid, std::shared_ptr<Session> session) {
    std::unique_lock<std::shared_mutex> lock(_mtx);
    _uid_to_session[uid] = session;
}


void UserManager::UnBind(int uid) {
    std::unique_lock<std::shared_mutex> lock(_mtx);
    _uid_to_session.erase(uid);
}

std::shared_ptr<Session> UserManager::GetSession(int uid) {
    std::shared_lock<std::shared_mutex> lock(_mtx);
    auto                                it = _uid_to_session.find(uid);
    if (it == _uid_to_session.end()) {
        LOG_INFO("[UserManager] the uid:{}'s session not found", uid);
        return nullptr;
    } else {
        return it->second;
    }
}
