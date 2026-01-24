#ifndef CHANNELPOOL_H_
#define CHANNELPOOL_H_

#include "infra/LogManager.h"
#include <atomic>
#include <cstddef>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>
#include <memory>
#include <string>
#include <vector>


using grpc::Channel;

class ChannelPool {
public:
    explicit ChannelPool(const std::string& target, std::size_t poolSize = 2)
        : _idx(0) {
        for (size_t i = 0; i < poolSize; ++i) {
            _channels.emplace_back(
                grpc::CreateChannel(
                    target, grpc::InsecureChannelCredentials()));
        }
        LOG_INFO("ChannelPool Created");
    }

    std::shared_ptr<Channel> get() {
        if (_channels.empty()) {
            return nullptr;
        }
        std::size_t idx     = _idx.fetch_add(1, std::memory_order_relaxed);
        auto        channel = _channels[idx % _channels.size()];
        return channel;
    }


private:
    std::vector<std::shared_ptr<Channel>> _channels;
    std::atomic<std::size_t>              _idx;
};


#endif   // CHANNELPOOL_H_
