#include "StatusServiceImpl.h"
#include "common/ChatServerInfo.h"
#include "common/UserMessage.h"
#include "common/const.h"
#include "infra/ConfigManager.h"
#include "infra/LogManager.h"
#include "infra/RedisManager.h"
#include "repository/ChatServerRepository.h"
#include <boost/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <grpcpp/support/status.h>
#include <mutex>
#include <sstream>
#include <string>

std::string generate_unique_string() {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return boost::uuids::to_string(uuid);
}

void StatusServiceImpl::InsertToken(int uid, const std::string& token) {
    auto uid_str = std::to_string(uid);
    RedisManager::getInstance()->Set(USER_UID_PREFIX + uid_str, token);
}

ChatServerInfo StatusServiceImpl::SelectChatServer() {
    std::lock_guard<std::mutex> lock(_server_mtx);
    int                         MIN = INT_MAX;
    ChatServerInfo              best_server;
    for (const auto& [name, server] : _servers) {
        if(!ChatServerRepository::isServerActivated(name)) { // 如果当前的服务器没有在线，就不注册当前服务器的信息
            LOG_WARN("[StatusServer] {} is offline! ", name);
            continue;
        }
        auto count = ChatServerRepository::GetConnectionCount(name);
        if(count < MIN) {
            MIN = count;
            best_server = server;
        }
    }
    LOG_INFO("best server is: {}, connection count is: {}", best_server.name, MIN);
    ChatServerRepository::IncrConnection(best_server.name);
    return best_server;
}

Status StatusServiceImpl::GetChatServer(
    ServerContext* context, const GetChatServerReq* request,
    GetChatServerRsp* reply) {

    if (_servers.empty()) {
        return Status(
            grpc::StatusCode::UNAVAILABLE, "No chat server available");
    }

    auto server = SelectChatServer();

    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_error(ErrorCode::SUCCESS);
    reply->set_token(generate_unique_string());

    InsertToken(request->uid(), reply->token());

    return Status::OK;
}

Status StatusServiceImpl::Login(
    ServerContext* context, const LoginReq* request, LoginRsp* reply) {
    auto uid   = request->uid();
    auto token = request->token();
    // Redis
    std::string redis_token = "";
    auto        uid_str     = USER_UID_PREFIX;
    uid_str += std::to_string(uid);
    if (RedisManager::getInstance()->Get(uid_str, redis_token)) {
        if (redis_token == token) {
            reply->set_error(ErrorCode::SUCCESS);
            reply->set_uid(uid);
            reply->set_token(token);
            return Status::OK;
        } else {
            reply->set_error(ErrorCode::TOKEN_INVALID);
            return Status::OK;
        }
    } else {
        reply->set_error(ErrorCode::REDIS_ERROR);
        return Status::OK;
    }
}
StatusServiceImpl::StatusServiceImpl() {
    auto              globalConfig = ConfigManager::getInstance();
    auto              server_list  = (*globalConfig)["ChatServers"]["name"];
    std::stringstream ssin(server_list);
    std::string       name;
    std::vector<std::string> names;
    while (std::getline(ssin, name, ',')) {
        names.push_back(name);
    }

    for (auto& server_name : names) {
        if ((*globalConfig)[server_name]["name"].empty()) {
            continue;
        }
        auto           host = (*globalConfig)[server_name]["host"];
        auto           port = (*globalConfig)[server_name]["port"];
        auto           name = (*globalConfig)[server_name]["name"];
        ChatServerInfo server;
        server.host           = host;
        server.port           = port;
        server.name           = name;
        _servers[server.name] = server;
        LOG_INFO("name is {} , host is {}, port is {}", name, host, port);
    }
}
