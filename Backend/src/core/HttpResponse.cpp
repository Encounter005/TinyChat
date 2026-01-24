#include "HttpResponse.h"
#include "HttpConnection.h"
#include "common/const.h"
#include "infra/LogManager.h"
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/status.hpp>
#include <json/value.h>


void HttpResponse::OK(
    std::shared_ptr<HttpConnection> connection, const Json::Value& data,
    const std::string& msg) {
    Json::Value body;
    body["error"] = 0;
    body["msg"]   = msg;
    body["data"]  = data;
    SendJson(connection, http::status::ok, body);
}
void HttpResponse::Error(
    std::shared_ptr<HttpConnection> connection, ErrorCodes code,
    const std::string& msg) {
    Json::Value body;
    body["error"] = static_cast<int>(code);
    body["msg"]   = msg.empty() ? ErrorMsg(code) : msg;
    SendJson(connection, http::status::bad_request, body);
}

void HttpResponse::SendJson(
    std::shared_ptr<HttpConnection> connection, http::status http_status,
    const Json::Value& body) {
    connection->GetResponse().result(http_status);
    connection->GetResponse().set(
        http::field::content_type, "application/json");
    beast::ostream(connection->GetResponse().body()) << body.toStyledString();
}
