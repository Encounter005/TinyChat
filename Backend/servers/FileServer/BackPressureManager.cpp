#include "BackPressureManager.h"
#include "infra/LogManager.h"
#include <mutex>


void BackPressureManager::wait_if_needed(size_t chunk_size) {
    std::unique_lock<std::mutex> lock(_mutex);
    LOG_WARN(
        "[Backpressure] In-flight bytes ({}) > threshold. Waiting...",
        _in_flight_bytes.load() + chunk_size);
    _cv.wait(lock, [&]() {
        return _in_flight_bytes + chunk_size <= MAX_IN_FLIGHT_BYTES;
    });
    LOG_INFO("[Backpressure] Resuming...");
    _in_flight_bytes += chunk_size;
}


void BackPressureManager::notify_data_flushed(size_t flushed_size) {
    if (flushed_size == 0) {
        return;
    }

    _in_flight_bytes -= flushed_size;
    _cv.notify_one();
}

void BackPressureManager::notify_all() {
    _cv.notify_all();
}
