#ifndef HTTPRESPONSE_H_
#define HTTPRESPONSE_H_

#include "common/const.h"
#include "common/result.h"
#include <json/value.h>
#include <memory>
#include <string>
class HttpConnection;

class HttpResponse {
public:
    static void OK(
        std::shared_ptr<HttpConnection> connection,
        const Json::Value& data = Json::Value(), const std::string& msg = "");

    static void Error(
        std::shared_ptr<HttpConnection> connection, ErrorCodes code,
        const std::string& msg = "");


private:
    static void SendJson(
        std::shared_ptr<HttpConnection> connection, http::status http_status,
        const Json::Value& body);
};


#endif   // HTTPRESPONSE_H_
