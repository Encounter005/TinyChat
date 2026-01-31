#ifndef BACKPRESSUREMANAGER_H_
#define BACKPRESSUREMANAGER_H_

#include "common/singleton.h"
#include <atomic>
#include <condition_variable>
#include <type_traits>
class BackPressureManager : public SingleTon<BackPressureManager> {
    friend class SingleTon<BackPressureManager>;

public:
        void wait_if_needed(size_t chunk_size);
        void notify_data_flushed(size_t flushed_size);
        void notify_all();
private:
    BackPressureManager() = default;
    std::atomic<size_t>     _in_flight_bytes{0};
    std::mutex              _mutex;
    std::condition_variable _cv;
    static const size_t MAX_IN_FLIGHT_BYTES = 1024 * 1024 * 1024 ;   // 1GB
};


#endif   // BACKPRESSUREMANAGER_H_
