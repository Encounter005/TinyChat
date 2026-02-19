#include "UserManager.h"
#include "infra/LogManager.h"
#include <memory>
#include <mutex>
#include <shared_mutex>

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


void UserManager::KickUser(int uid, const std::string& reason) {
    auto old_session = GetSession(uid);
    if (!old_session) {
        LOG_INFO("[UserManager] Cannot kick user {}, session not found", uid);
        return;
    }

    // 构造 JSON 通知消息
    Json::Value notify_root;
    notify_root["error"] = static_cast<int>(ErrorCodes::SUCCESS);
    notify_root["msg"]   = reason;
    notify_root["uid"]   = uid;

    std::string notify_body = notify_root.toStyledString();

    LOG_INFO("[UserManager] Kicking user {}, reason: {}", uid, reason);

    // 调用 Session 的带通知关闭方法
    old_session->CloseWithNotify(MsgId::ID_NOTIFY_OFF_LINE_REQ, notify_body);
}
