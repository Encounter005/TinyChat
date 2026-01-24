#include "ChatServer.h"
#include "LogicHandler.h"
#include "Messsage.h"
#include "SessionManager.h"
#include "UserManager.h"
#include "common/ChatServerInfo.h"
#include "common/UserMessage.h"
#include "common/const.h"
#include "dispatcher.h"
#include "grpcClient/StatusClient.h"
#include "infra/AsioIOServicePool.h"
#include "infra/LogManager.h"
#include "service/UserService.h"
#include "session.h"
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <json/reader.h>
#include <json/value.h>
#include <memory>
#include <string>
#include <sys/types.h>

ChatServer::ChatServer(
    boost::asio::io_context& accept_ioc, unsigned short port,
    const ChatServerInfo& server_info)
    : _accept_ioc(accept_ioc)
    , _acceptor(
          _accept_ioc,
          boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    , _heartbeat_timer(_accept_ioc)
    , _dispatcher(std::make_shared<Dispatcher>())
    , _server_info(server_info) {
    LOG_INFO("[ChatServer] listening the port: {}", port);
    Register();
    DoAccept();
    // StartHeartBeat(); // 心跳检测
}

void ChatServer::Register() {
    // Echo test
    _dispatcher->Register(MsgId::ID_ECHO, [](auto session, const auto& msg) {
        LogicHandler::HelloEcho(session, msg);
    });
    // Login
    _dispatcher->Register(
        MsgId::ID_CHAT_LOGIN, [this](auto session, const auto& msg) {
            LogicHandler::HandleLogin(this->_server_info, session, msg);
        });
}

void ChatServer::DoAccept() {
    auto& ioc     = AsioIOServicePool::getInstance()->GetIOService();
    auto  session = std::make_shared<Session>(ioc, _server_info.name);
    session->SetDispatcher(_dispatcher);
    session->SetCloseCallback(
        [mgr = SessionManager::getInstance()](const std::string& id) {
            mgr->Remove(id);
            LOG_INFO("session {} closed", id);
        });

    _acceptor.async_accept(
        session->Socket(),
        [this, session](const boost::system::error_code& ec) {
            if (!ec) {
                SessionManager::getInstance()->Add(session);
                session->Start();
            }
            DoAccept();
        });
}

void ChatServer::StartHeartBeat() {
    _heartbeat_timer.expires_after(std::chrono::seconds(5));
    _heartbeat_timer.async_wait([this](const boost::system::error_code& ec) {
        if (!ec) {
            SessionManager::getInstance()->CheckTimeouts(30);
            StartHeartBeat();
        }
    });
}
