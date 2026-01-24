#include "ChatClient.h"
#include "common/ChatServerInfo.h"
#include "infra/ConfigManager.h"
#include "message.pb.h"
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
        if ((*globalconfig)[server_name]["namem"].empty()) {
            continue;
        }
        auto host = (*globalconfig)[server_name]["host"];
        auto port = (*globalconfig)[server_name]["port"];
        auto name = (*globalconfig)[server_name]["name"];

        _pools[name] = std::make_shared<ChannelPool>(host + ":" + port, 4);

    }
}

AddFriendRsp ChatClient::NotifyAddFriend(
    const std::string& server_ip, const AddFriendReq& req) {
    AddFriendRsp rsp;
    return rsp;
}

AuthFriendRsp ChatClient::NotifyAuthFriend(
    const std::string& server_ip, const AuthFriendReq& req) {
    AuthFriendRsp rsp;
    return rsp;
}

bool ChatClient::GetBaseInfo(
    const std::string& base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
    return true;
}

TextChatMsgRsp ChatClient::NotifyTextChatMsg(
    const std::string& server_ip, const TextChatMsgReq& req,
    const Json::Value& res) {
    TextChatMsgRsp rsp;
    return rsp;
}
