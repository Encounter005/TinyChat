#ifndef SESSION_H_
#define SESSION_H_
#include "dispatcher.h"
#include <array>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
using boost::asio::ip::tcp;
using CloseCallback = std::function<void(const std::string&)>;
using Clock         = std::chrono::steady_clock;

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(
        boost::asio::io_context& ioc, const std::string& server_name);

    tcp::socket&       Socket();
    void               Start();
    void               Send(uint16_t msg_id, const std::string& body);
    void               Send(MsgId msg_id, const std::string& body);
    void               PostClose();
    void               SetDispatcher(std::shared_ptr<Dispatcher> dispatcher);
    void               SetCloseCallback(CloseCallback cb);
    void               SetUid(int uid);
    Clock::time_point  LastActive() const;
    const std::string& Id() const;

private:
    void DoRead();
    void OnRead(const boost::system::error_code&, std::size_t);

    void DoClose();
    void DoWrite();
    void OnWrite(const boost::system::error_code&, std::size_t);
    void ParsePackets();
    void OnMessage(const Message&);

private:
    tcp::socket                                                 _socket;
    boost::asio::strand<boost::asio::io_context::executor_type> _strand;

    std::array<char, 4096>      _read_buf;
    std::vector<char>           _recv_buffer;
    std::deque<std::string>     _write_queue;
    uint32_t                    _expected_len{0};
    std::shared_ptr<Dispatcher> _dispatcher;
    std::string                 _uuid;
    std::atomic<bool>           _closed{false};
    CloseCallback               _on_close;
    std::atomic<int64_t>        _last_active;
    int                         _user_uid;
    std::string                 _server_name;
};


#endif   // SESSION_H_
