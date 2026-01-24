#ifndef CHATSERVER_H_
#define CHATSERVER_H_

#include "SessionManager.h"
#include "common/ChatServerInfo.h"
#include "dispatcher.h"
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/detail/error_code.hpp>
#include <memory>

using boost::asio::io_context;
using boost::asio::ip::tcp;

class ChatServer {
public:
    ChatServer (boost::asio::io_context& accept_ioc, unsigned short port, const ChatServerInfo& server_info);

private:
    void DoAccept();
    void StartHeartBeat();
    void Register();

private:
    boost::asio::io_context& _accept_ioc;
    boost::asio::ip::tcp::acceptor _acceptor;
    boost::asio::steady_timer _heartbeat_timer;
    std::shared_ptr<Dispatcher> _dispatcher;
    ChatServerInfo _server_info;
};

#endif   // CHATSERVER_H_
