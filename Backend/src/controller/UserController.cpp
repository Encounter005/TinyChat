#include "UserController.h"
#include "common/const.h"
#include "core/HttpConnection.h"
#include "core/HttpResponse.h"
#include "infra/LogManager.h"
#include "grpcClient/StatusClient.h"
#include "service/UserService.h"
#include <boost/beast/core/buffers_to_string.hpp>
#include <json/reader.h>
#include <json/value.h>
#include <memory>

void UserController::Register(LogicSystem &logic) {
    logic.RegPost("/user_register", RegisterUser);
}

void UserController::ResetPwd(LogicSystem &logic) {
    logic.RegPost("/reset_pwd", ResetUserPwd);
}

void UserController::Login(LogicSystem &logic) {
    logic.RegPost("/user_login", UserLogin);
}


void UserController::RegisterUser(std::shared_ptr<HttpConnection> connection) {

    auto body = boost::beast::buffers_to_string(
        connection->GetRequest().body().data());

    Json::Value  src, rsp;
    Json::Reader reader;
    if (!reader.parse(body, src)) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON);
        return;
    }

    auto res = UserService::Register(
        src["user"].asString(),
        src["email"].asString(),
        src["passwd"].asString(),
        src["confirm"].asString(),
        src["varifycode"].asString());


    if(res.IsOK()) {
        HttpResponse::OK(connection);
    } else {
        HttpResponse::Error(connection, res.Error());
    }

}



void UserController::ResetUserPwd(std::shared_ptr<HttpConnection> connection) {
    auto body
        = beast::buffers_to_string(connection->GetRequest().body().data());

    Json::Value  src, rsp;
    Json::Reader reader;
    if (!reader.parse(body, src)) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON);
        return;
    }

    auto res = UserService::ResetPass(
        src["user"].asString(),
        src["email"].asString(),
        src["passwd"].asString(),
        src["varifycode"].asString());

    if(res.IsOK()) {
        HttpResponse::OK(connection);
    } else {
        HttpResponse::Error(connection, res.Error());
    }

}


void UserController::UserLogin(std::shared_ptr<HttpConnection> connection) {
    auto body
        = beast::buffers_to_string(connection->GetRequest().body().data());
    Json::Value  src, data;
    Json::Reader reader;
    if (!reader.parse(body, src)) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON);
        return;
    }

    auto res = UserService::Login(
        src["email"].asString());

    UserInfo userInfo = res.Value();
    auto passwd = src["passwd"].asString();
    if(passwd != userInfo.pwd) {
        HttpResponse::Error(connection, ErrorCodes::PASSWORD_ERROR);
        return;
    }

    auto reply = StatusClient::getInstance()->GetChatServer(userInfo.uid);
    if (reply.error()) {
        LOG_ERROR("[StatusClient] get chat server failed");
        HttpResponse::Error(connection, ErrorCodes::RPC_FAILED);
    }

    LOG_INFO("successd to load user_info uid is: {}", userInfo.uid);
    data["token"] = reply.token();
    data["name"]  = userInfo.name;
    data["host"]  = reply.host();
    data["uid"] = userInfo.uid;
    data["port"] = reply.port();

    HttpResponse::OK(connection, data);
}
