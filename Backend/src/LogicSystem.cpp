#include "LogicSystem.h"
#include "HttpConnection.h"
#include "LogManager.h"
#include "VarifyGrpcClient.h"


LogicSystem::LogicSystem() {
    RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
        beast::ostream(connection->_response.body())
            << "receive get_test req " << std::endl;
        int i = 0;
        for (const auto& elem : connection->_get_params) {
            i++;
            beast::ostream(connection->_response.body())
                << "param " << i << " key is: " << elem.first;
            beast::ostream(connection->_response.body())
                << " value is: " << elem.second << std::endl;
        }
    });

    RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(
            connection->_request.body().data());
        LOG_INFO("receive body data is: {}", body_str);
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value  root;
        Json::Reader reader;
        Json::Value  src_root;
        bool         parse_flag = reader.parse(body_str, src_root);
        if (!parse_flag) {
            LOG_WARN("Faild to parse Json data!");
            root["error"]        = ErrorCodes::ERROR_JSON;
            std::string json_str = root.toStyledString();
            beast::ostream(connection->_response.body()) << json_str;
            return;
        }

        auto email = src_root["email"].asString();
        LOG_INFO("email is: {}", email );
        GetVarifyRsp rsp = VarifyGrpcClient::getInstance()->GetVarifyCode(email);

        root["error"]        = rsp.error();
        root["email"]        = email;
        std::string json_str = root.toStyledString();
        beast::ostream(connection->_response.body()) << json_str;
    });
}

bool LogicSystem::HandleGet(
    const std::string& path, std::shared_ptr<HttpConnection> connection) {
    if (_get_handlers.find(path) == _get_handlers.end()) {
        // url not found
        return false;
    }
    _get_handlers[path](connection);
    return true;
}

bool LogicSystem::HandlePost(
    const std::string& path, std::shared_ptr<HttpConnection> connection) {
    if(_post_handlers.find(path) == _post_handlers.end()) {
        // url not found
        return false;
    }
    _post_handlers[path](connection);
    return true;
}



void LogicSystem::RegGet(const std::string& url, HttpHandler handler) {
    _get_handlers.insert(std::make_pair(url, handler));
}

void LogicSystem::RegPost(const std::string& url, HttpHandler handler) {
    _post_handlers.insert(std::make_pair(url, handler));
}
