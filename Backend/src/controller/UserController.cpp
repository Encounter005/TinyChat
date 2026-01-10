#include "UserController.h"
#include "core/HttpConnection.h"
#include "core/HttpResponse.h"
#include "service/UserService.h"
#include "common/const.h"
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


void UserController::RegisterUser(std::shared_ptr<HttpConnection> connection) {

    auto body = boost::beast::buffers_to_string(connection->GetRequest().body().data());

    Json::Value src, rsp;
    Json::Reader reader;
    if(!reader.parse(body, src)) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON);
        return;
    }

    int err = UserService::Register(src["user"].asString(), src["email"].asString(), src["passwd"].asString(), src["confirm"].asString(), src["varifycode"].asString());

    if(err != 0) {
        HttpResponse::Error(connection, err);
        return;
    }

    rsp["user"] = src["user"];
    rsp["email"] = src["email"];
    HttpResponse::OK(connection, rsp);
}


void UserController::ResetUserPwd(std::shared_ptr<HttpConnection> connection) {
    auto body = beast::buffers_to_string(connection->GetRequest().body().data());

    Json::Value src, rsp;
    Json::Reader reader;
    if(!reader.parse(body, src)) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON);
        return;
    }

    int err = UserService::ResetPass(src["user"].asString(), src["email"].asString(), src["passwd"].asString() ,src["varifycode"].asString());
    if(err != ErrorCodes::SUCCESS) {
        HttpResponse::Error(connection, err);
        return;
    }

    rsp["user"] = src["user"];
    rsp["email"] = src["email"];
    HttpResponse::OK(connection, rsp);

}
