#ifndef SESSIONMANAGER_H_
#define SESSIONMANAGER_H_

#include "common/singleton.h"
#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Session;
class SessionManager : public SingleTon<SessionManager> {
    friend class SingleTon<SessionManager>;

public:
    static constexpr int     BUCKET_COUNT = 32;
    void                     Add(const std::shared_ptr<Session>& session);
    void                     Remove(const std::string& uuid);
    void                     CheckTimeouts(int max_idle_s);
    std::shared_ptr<Session> Get(const std::string& uuid);

    template<typename Func> void ForEach(Func&& func) {
        for (int i = 0; i < BUCKET_COUNT; ++i) {
            std::vector<std::shared_ptr<Session>> snapshot;
            {
                std::lock_guard<std::mutex> lock(_buckets[i].mtx);
                for (auto& pair : _buckets[i].sessions) {
                    snapshot.push_back(pair.second);
                }
            }   // release lock

            for (auto& session : snapshot) {
                func(session);
            }
        }
    }   // broadcast

private:
    struct Bucket {
        std::mutex                                                mtx;
        std::unordered_map<std::string, std::shared_ptr<Session>> sessions;
    };
    std::array<Bucket, BUCKET_COUNT> _buckets;

private:
    Bucket& GetBucket(const std::string& key) {
        std::size_t index = std::hash<std::string>{}(key) % BUCKET_COUNT;
        return _buckets[index];
    }
    SessionManager() = default;
};


#endif   // SESSIONMANAGER_H_
