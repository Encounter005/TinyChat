#ifndef STATUSCLIENT_H_
#define STATUSCLIENT_H_
#include "common/const.h"
#include "common/singleton.h"
#include "infra/ChannelPool.h"
#include "infra/StubFactory.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>
#include <memory>

using grpc::ClientContext;
using grpc::Status;
using message::ErrorCode;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

class StatusClient : public SingleTon<StatusClient> {
    friend class SingleTon<StatusClient>;

public:
    ~StatusClient() = default;
    GetChatServerRsp GetChatServer(int uid);
    LoginRsp         Login(int uid, const std::string& token);

private:
    StatusClient();
    std::shared_ptr<ChannelPool>                      _pool;
    std::unique_ptr<StubFactory<StatusService>> _stubFactory;
};

#endif   // STATUSCLIENT_H_
