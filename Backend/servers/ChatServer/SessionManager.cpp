#include "SessionManager.h"
#include "infra/LogManager.h"
#include "session.h"
#include <chrono>
#include <mutex>

void SessionManager::Add(const std::shared_ptr<Session>& session) {
    auto&                       bucket = GetBucket(session->Id());
    std::lock_guard<std::mutex> lock(bucket.mtx);
    bucket.sessions[session->Id()] = session;
    LOG_INFO("[SessionManager] add new session, uuid is: {}", session->Id());
}

void SessionManager::Remove(const std::string& uuid) {
    auto&                       bucket = GetBucket(uuid);
    std::lock_guard<std::mutex> lock(bucket.mtx);
    bucket.sessions.erase(uuid);
    LOG_INFO("[SessionManager] remove uuid: {}", uuid);
}

std::shared_ptr<Session> SessionManager::Get(const std::string& uuid) {
    auto&                       bucket = GetBucket(uuid);
    std::lock_guard<std::mutex> lock(bucket.mtx);
    auto                        it = bucket.sessions.find(uuid);
    return it == bucket.sessions.end() ? nullptr : it->second;
}

void SessionManager::CheckTimeouts(int max_idle_s) {
    auto now = std::chrono::steady_clock::now();
    this->ForEach([&](const std::shared_ptr<Session>& session) {
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(
            now - session->LastActive());
        int idle_seconds = diff.count();

        if (idle_seconds >= max_idle_s + 5) {
            // 强制断开
            LOG_WARN(
                "[HeartBeat] Session {} idle for {}s, closing after probe "
                "timeout",
                session->Id(),
                idle_seconds);
            session->PostClose();
        } else if (idle_seconds >= max_idle_s) {
            // 达到超时阈值，检查是否需要探测
            if (session->NeedsProbing(idle_seconds)) {
                LOG_WARN(
                    "[HeartBeat] Session {} idle for {}s, sending heartbeat "
                    "probe",
                    session->Id(),
                    idle_seconds);
                session->SendHeartbeatProbe();
            }
        } else if (idle_seconds >= max_idle_s - 15) {
            // 接近超时：标记为可疑状态
            LOG_INFO(
                "[HeartBeat] Session {} suspicious, idle for {}s",
                session->Id(),
                idle_seconds);
        }
    });
}
