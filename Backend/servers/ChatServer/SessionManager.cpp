#include "SessionManager.h"
#include "session.h"
#include <chrono>
#include <mutex>
#include "infra/LogManager.h"

void SessionManager::Add(const std::shared_ptr<Session>& session) {
    auto& bucket = GetBucket(session->Id());
    std::lock_guard<std::mutex> lock(bucket.mtx);
    bucket.sessions[session->Id()] = session;
    LOG_INFO("[SessionManager] add new session, uuid is: {}", session->Id());
}

void SessionManager::Remove(const std::string& uuid) {
    auto& bucket = GetBucket(uuid);
    std::lock_guard<std::mutex> lock(bucket.mtx);
    bucket.sessions.erase(uuid);
    LOG_INFO("[SessionManager] remove uuid: {}", uuid);
}

std::shared_ptr<Session> SessionManager::Get(const std::string& uuid) {
    auto& bucket = GetBucket(uuid);
    std::lock_guard<std::mutex> lock(bucket.mtx);
    auto it = bucket.sessions.find(uuid);
    return it == bucket.sessions.end() ? nullptr : it->second;
}

void SessionManager::CheckTimeouts(int max_idle_s) {
   auto now = std::chrono::steady_clock::now();
   this->ForEach([&](const std::shared_ptr<Session>& session){
       auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - session->LastActive());
       if(diff.count() > max_idle_s) {
           LOG_WARN("[HeartBeat] Session {} timeout, last active {}s ago", session->Id(), diff.count());
           session->PostClose();
       }
   });
}
