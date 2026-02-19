#include "ChatServer.h"
#include "LogicHandler.h"
#include "MessagePersistenceService.h"
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
#include "repository/ChatServerRepository.h"
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

    _persistence_service = std::make_shared<MessagePersistenceService>(_accept_ioc, 5);
    _persistence_service->Start();

    Register();
    DoAccept();
    StartHeartBeat();   // 心跳检测
}

void ChatServer::Register() {
    // Echo test
    _dispatcher->Register(MsgId::ID_ECHO, [](auto session, const auto& msg) {
        LogicHandler::HelloEcho(session, msg);
    });
    // Login
    _dispatcher->Register(
        MsgId::ID_CHAT_LOGIN_REQ, [this](auto session, const auto& msg) {
            LogicHandler::HandleLogin(this->_server_info, session, msg);
        });
    // Search
    _dispatcher->Register(
        MsgId::ID_SEARCH_USER_REQ, [](auto session, const auto& msg) {
            LogicHandler::HandleSearch(session, msg);
        });
    // ApplyFriend
    _dispatcher->Register(
        MsgId::ID_ADD_FRIEND_REQ, [this](auto session, const auto& msg) {
            LogicHandler::AddFriendApply(this->_server_info, session, msg);
        });
    // AuthFriend
    _dispatcher->Register(
        MsgId::ID_AUTH_FRIEND_REQ, [this](auto session, const auto& msg) {
            LogicHandler::AuthFriendApply(this->_server_info, session, msg);
        });
    // TextMsg
    _dispatcher->Register(
        MsgId::ID_TEXT_CHAT_MSG_REQ, [this](auto session, const auto& msg) {
            LogicHandler::HandleChatTextMsg(this->_server_info, session, msg);
        });
    // Heartbeat
    _dispatcher->Register(
        MsgId::ID_HEART_BEAT_REQ, [this](auto session, const auto& msg) {
            LogicHandler::HandleHeartBeat(session, msg);
        });
}

void ChatServer::DoAccept() {
    auto& ioc     = AsioIOServicePool::getInstance()->GetIOService();
    auto  session = std::make_shared<Session>(ioc, _server_info.name);
    session->SetDispatcher(_dispatcher);
    session->SetHeartbeatConfig(
        _server_info.heartbeat_timeout, _server_info.heartbeat_probe_wait);
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
    _heartbeat_timer.expires_after(
        std::chrono::seconds(_server_info.heartbeat_check_interval));
    _heartbeat_timer.async_wait([this](const boost::system::error_code& ec) {
        if (!ec) {
            SessionManager::getInstance()->CheckTimeouts(
                _server_info.heartbeat_timeout);
            StartHeartBeat();
        }
    });
}

ChatServer::~ChatServer() {
    if (_persistence_service) {
        LOG_INFO("[ChatServer] Flushing cached messages before shutdown");
        _persistence_service->Stop();
        _persistence_service->ForcePersistOnce();
    }
}
