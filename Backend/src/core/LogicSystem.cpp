#include "LogicSystem.h"
#include "HttpConnection.h"
#include "Router.h"
#include "VarifyGrpcClient.h"
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/ostream.hpp>
#include <json/reader.h>



LogicSystem::LogicSystem() {
    Router::RegisterRoutes(*this);
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
    if (_post_handlers.find(path) == _post_handlers.end()) {
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
