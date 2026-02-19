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
    void               CloseWithNotify(MsgId msg_id, const std::string& body);
    void               SetDispatcher(std::shared_ptr<Dispatcher> dispatcher);
    void               SetCloseCallback(CloseCallback cb);
    void               SetUid(int uid);
    Clock::time_point  LastActive() const;
    const std::string& Id() const;
    void               MarkAsLoggedIn(const std::string& server_name);
    void               OnHeartBeatRequest();     // 处理客户端心跳检测
    void               SendHeartbeatProbe();     // 发送探测包
    bool NeedsProbing(int idle_seconds) const;   // 检查是否需要探测
    void OnProbeTimeout();                       // 探测超时处理
    void SetHeartbeatConfig(int timeout, int probe_wait);


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
    std::atomic<bool>           _close_after_write{false};
    std::atomic<bool>           _login_counted{false};
    CloseCallback               _on_close;
    std::atomic<int64_t>        _last_active;
    int                         _user_uid;
    std::string                 _server_name;

    enum class HeartbeatState {
        NORMAL,
        SUSPICIOUS,   // 疑似超时状态
        PROBING,      // 探测状态
        TIMEOUT,      // 超时状态
    };

    std::atomic<HeartbeatState> _hb_state{HeartbeatState::NORMAL};
    std::atomic<int64_t>        _last_probe_time{0};   // 上次探测时间戳
    int _heartbeat_timeout = 60;   // 超时时间（从 ChatServer 传入）
    int _probe_wait_time   = 5;    // 探测等待时间
};


#endif   // SESSION_H_
