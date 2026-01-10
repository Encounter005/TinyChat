#include "HttpResponse.h"
#include "HttpConnection.h"
#include "common/const.h"
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/status.hpp>
#include <json/value.h>

static std::string ErrorMsg(int code) {
    switch(code) {
        case ErrorCodes::ERROR_JSON:
            return "json parse error";
        case ErrorCodes::PasswdError:
            return "password error";
        case ErrorCodes::UserExist:
            return "user already exists";
        case ErrorCodes::VarifyExpired:
            return "varify code expired";
        case ErrorCodes::VarifyCodeError:
            return "varify code error";
        case ErrorCodes::EmailNotMatch:
            return "email not match";
        default:
            return "unknown error";
    }
}

void HttpResponse::OK(std::shared_ptr<HttpConnection> connection, const Json::Value& data, const std::string& msg) {
    Json::Value body;
    body["error"] = 0;
    body["msg"] = msg;
    body["data"] = data;
    SendJson(connection, http::status::ok, body);
}
void HttpResponse::Error(std::shared_ptr<HttpConnection> connection, int error_code, const std::string& msg) {
    Json::Value body;
    body["error"] = error_code;
    body["msg"] = msg.empty() ? ErrorMsg(error_code) : msg;
    SendJson(connection, http::status::bad_request, body);
}


void HttpResponse::SendJson(
        std::shared_ptr<HttpConnection> connection, http::status http_status,
        const Json::Value& body) {
    connection->GetResponse().result(http_status);
    connection->GetResponse().set(http::field::content_type, "application/json");
    beast::ostream(connection->GetResponse().body()) << body.toStyledString();
}
