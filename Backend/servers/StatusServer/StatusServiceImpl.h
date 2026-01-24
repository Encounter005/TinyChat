#ifndef STATUSSERVICEIMPL_H_
#define STATUSSERVICEIMPL_H_

#include "common/ChatServerInfo.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>
#include <unordered_map>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::ErrorCode;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;



class StatusServiceImpl final : public StatusService::Service {
public:
    StatusServiceImpl();
    Status GetChatServer(
        ServerContext* context, const GetChatServerReq* request,
        GetChatServerRsp* reply) override;
    Status Login(
        ServerContext* context, const LoginReq* request,
        LoginRsp* reply) override;

private:
    void           InsertToken(int uid, const std::string& token);
    ChatServerInfo SelectChatServer();

private:
    std::unordered_map<std::string, ChatServerInfo> _servers;
    std::unordered_map<int, std::string>            _tokens;
    std::mutex                                      _server_mtx;
    std::mutex                                      _token_mtx;
};


#endif   // STATUSSERVICEIMPL_H_
