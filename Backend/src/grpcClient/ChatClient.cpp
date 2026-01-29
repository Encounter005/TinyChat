#include "ChatClient.h"
#include "common/ChatServerInfo.h"
#include "common/const.h"
#include "grpcClient/StatusClient.h"
#include "infra/ConfigManager.h"
#include "infra/Defer.h"
#include "infra/LogManager.h"
#include "infra/StubFactory.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/client_context.h>
#include <sstream>

ChatClient::ChatClient() {
    auto              globalconfig = ConfigManager::getInstance();
    auto              server_list  = (*globalconfig)["ChatServers"]["name"];
    std::stringstream ssin(server_list);
    std::vector<std::string> server_names;
    std::string              name;
    while (std::getline(ssin, name, ',')) {
        server_names.push_back(name);
    }

    for (auto& server_name : server_names) {
        // LOG_DEBUG("========HELLO==========");
        if ((*globalconfig)[server_name]["name"].empty()) {
            continue;
        }

        auto host = (*globalconfig)[server_name]["host"];
        auto port = (*globalconfig)[server_name]["RPCport"];
        auto name = (*globalconfig)[server_name]["name"];

        _pools[name] = std::make_shared<ChannelPool>(host + ":" + port, 4);
    }
}

AddFriendRsp ChatClient::NotifyAddFriend(
    const std::string& server_name, const AddFriendReq& req) {
    AddFriendRsp             rsp;
    ClientContext            context;
    StubFactory<ChatService> stubFactory(_pools.at(server_name));
    auto                     stub = stubFactory.create();
    Status status                 = stub->NotifyAddFriend(&context, req, &rsp);
    if (!status.ok()) {
        rsp.set_error(ErrorCode::RPC_FAILED);
    } else {
        rsp.set_error(ErrorCode::SUCCESS);
        rsp.set_applyuid(req.applyuid());
        rsp.set_touid(req.touid());
    }

    return rsp;
}

AuthFriendRsp ChatClient::NotifyAuthFriend(
    const std::string& server_name, const AuthFriendReq& req) {
    AuthFriendRsp rsp;

    ClientContext            context;
    StubFactory<ChatService> stubFactory(_pools.at(server_name));
    auto                     stub = stubFactory.create();
    Status status                 = stub->NotifyAuthFriend(&context, req, &rsp);
    if (!status.ok()) {
        rsp.set_error(ErrorCode::RPC_FAILED);
    }
    rsp.set_error(ErrorCode::SUCCESS);
    return rsp;
}

bool ChatClient::GetBaseInfo(
    const std::string& base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
    return true;
}

TextChatMsgRsp ChatClient::NotifyTextChatMsg(
    const std::string& server_name, const TextChatMsgReq& req,
    const Json::Value& res) {
    TextChatMsgRsp rsp;
    rsp.set_error(ErrorCode::SUCCESS);
    Defer defer([&req, &rsp]() {
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());
        for (const auto& text_data : req.textmsgs()) {
            TextChatData* new_msg = rsp.add_textmsgs();
            new_msg->set_msgid(text_data.msgid());
            new_msg->set_msgcontent(text_data.msgcontent());
        }
    });
    StubFactory<ChatService> stubFactory(_pools.at(server_name));
    auto                     stub = stubFactory.create();

    ClientContext context;
    Status        status = stub->NotifyTextChatmsg(&context, req, &rsp);

    if (!status.ok()) {
        rsp.set_error(ErrorCode::RPC_FAILED);
        return rsp;
    }

    return rsp;
}
