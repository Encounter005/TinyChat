#ifndef STUBFACTORY_H_
#define STUBFACTORY_H_

#include "infra/ChannelPool.h"
#include <memory>
template<typename Service> class StubFactory {
public:
    explicit StubFactory(std::shared_ptr<ChannelPool> channelPool)
        : _channelPool(channelPool) {}
    std::unique_ptr<typename Service::Stub> create() {
        return Service::NewStub(_channelPool->get());
    }

private:
    std::shared_ptr<ChannelPool> _channelPool;
};


#endif   // STUBFACTORY_H_
