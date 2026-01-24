#include "AuthController.h"
#include "core/HttpConnection.h"
#include "core/HttpResponse.h"
#include "infra/LogManager.h"
#include "grpcClient/VarifyClient.h"
#include "common/const.h"
#include <boost/beast/core/buffers_to_string.hpp>
#include <json/reader.h>
#include <memory>

void AuthController::Register(LogicSystem &logic) {
    logic.RegPost("/get_varifycode", GetVarifyCode);
}

void AuthController::GetVarifyCode(std::shared_ptr<HttpConnection> connection) {
    auto body = beast::buffers_to_string(connection->GetRequest().body().data());

    Json::Value src, rsp;
    Json::Reader reader;

    if(!reader.parse(body, src)) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON);
        return;
    }

    auto email = src["email"].asString();
    auto grpc_rsp = VarifyGrpcClient::getInstance()->GetVarifyCode(email);

    if(grpc_rsp.error() == ErrorCode::RPC_FAILED) {
        LOG_WARN("Occur RPC_FAILED when get varifycode");
        HttpResponse::Error(connection, ErrorCodes::RPC_FAILED);
        return;
    }
    LOG_INFO("Success get varifycode: {}", grpc_rsp.code());

    rsp["email"] = email;
    HttpResponse::OK(connection, rsp);
}
