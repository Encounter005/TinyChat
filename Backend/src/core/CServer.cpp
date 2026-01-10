#include "CServer.h"
#include "infra/AsioIOServicePool.h"
#include "core/HttpConnection.h"
#include "infra/LogManager.h"
#include <memory>

CServer::CServer(boost::asio::io_context &ioc, unsigned short &port)
    : _ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port)), _socket(ioc) {
    LOG_INFO("Server is listening port: {}", port);
}

void CServer::Start() {
    auto  self       = shared_from_this();
    auto &io_context = AsioIOServicePool::getInstance()->GetIOService();
    std::shared_ptr<HttpConnection> new_connection
        = std::make_shared<HttpConnection>(tcp::socket(io_context));
    _acceptor.async_accept(
        new_connection->GetSocket(),
        [self, new_connection](beast::error_code ec) {
            try {
                if (ec) {
                    LOG_ERROR("Accept error: {}, code: {}", ec.message(), ec.value());
                    self->Start();
                    return;
                }
                LOG_INFO("New client connected successfully.");
                new_connection->Start();
                self->Start();


            } catch (std::exception &e) {
                LOG_ERROR("In CServer::Start(), exception is: {}", e.what());
                self->Start();
            }
        });
}
