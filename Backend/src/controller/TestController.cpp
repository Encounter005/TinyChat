#include "TestController.h"
#include "core/HttpConnection.h"
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/ostream.hpp>
#include <memory>
#include "core/HttpResponse.h"

void TestController::Register(LogicSystem &logic) {
    logic.RegGet("/get_test", GetTest);
}

void TestController::GetTest(std::shared_ptr<HttpConnection> connection) {
    Json::Value rsp;
    for(const auto& elm : connection->GetParams()) {
        rsp[elm.first] = elm.second;
    }

    HttpResponse::OK(connection, rsp, "receive test data");
}
