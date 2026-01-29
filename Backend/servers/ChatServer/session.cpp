#include "session.h"
#include "Messsage.h"
#include "SessionManager.h"
#include "UserManager.h"
#include "common/const.h"
#include "dispatcher.h"
#include "infra/LogManager.h"
#include "infra/Defer.h"
#include "repository/ChatServerRepository.h"
#include "repository/UserRepository.h"
#include <boost/asio.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>

Session::Session(boost::asio::io_context& ioc, const std::string& server_name)
    : _socket(ioc), _strand(boost::asio::make_strand(ioc)), _user_uid(-1), _server_name(server_name) {
    auto id = boost::uuids::random_generator()();
    _uuid   = boost::uuids::to_string(id);
    _last_active.store(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now().time_since_epoch())
            .count());
}

tcp::socket& Session::Socket() {
    return _socket;
}

void Session::Start() {
    auto self = shared_from_this();
    boost::asio::post(_strand, [self]() { self->DoRead(); });
}

void Session::Send(MsgId msg_id, const std::string& msg) {
    Send(static_cast<uint16_t>(msg_id), msg);
}

void Session::Send(uint16_t msg_id, const std::string& msg) {
    auto self = shared_from_this();
    boost::asio::post(_strand, [self, msg_id, msg]() {
        if (self->_closed.load()) return;

        uint32_t len     = static_cast<uint32_t>(msg.size());
        uint32_t net_len = htonl(
            len);   // 将主机字节序转换成网络字节序，保证不同字节序的机器店铺能正确解析消息长度
        uint16_t net_msg_id = htons(msg_id);

        std::string packet;
        packet.resize(HEADER_LEN + HEADER_ID + len);
        memcpy(packet.data(), &net_len, HEADER_LEN);
        memcpy(packet.data() + HEADER_LEN, &net_msg_id, HEADER_ID);
        memcpy(packet.data() + HEADER_LEN + HEADER_ID, msg.data(), len);
        bool idle
            = self->_write_queue.empty();   // 保证只有一个async_write在运行
        self->_write_queue.emplace_back(std::move(packet));
        if (idle) {
            self->DoWrite();
        }
    });
}

void Session::DoClose() {
    auto self = shared_from_this();
    boost::asio::post(_strand, [self]() {
        // 1. 只允许关闭一次
        bool expected = false;
        if (!self->_closed.compare_exchange_strong(expected, true)) {
            return;
        }

        // 2. 最后一定要执行的清理（统一兜底）
        Defer defer_cleanup([self]() {
            // socket 关闭（一定执行）
            boost::system::error_code ec;
            self->_socket.shutdown(tcp::socket::shutdown_both, ec);
            self->_socket.close(ec);

            // session 移除
            SessionManager::getInstance()->Remove(self->Id());
            // server 连接数 -1
            ChatServerRepository::DecrConnection(self->_server_name);

            // 回调
            if (self->_on_close) {
                self->_on_close(self->_uuid);
            }
        });

        // 3. 用户解绑（可安全提前 return）
        if (self->_user_uid != -1) {
            UserManager::getInstance()->UnBind(self->_user_uid);
            // remove user uid with server
            UserRepository::UnBindUserIpWithServer(self->_user_uid);
            self->_user_uid = -1;
        }

        // 👇 以后你在这里随便加 return，都不会破坏清理
    });
}

void Session::DoRead() {
    auto self = shared_from_this();
    _socket.async_read_some(
        boost::asio::buffer(_read_buf),
        boost::asio::bind_executor(
            _strand, [self](const boost::system::error_code& ec, std::size_t bytes) { self->OnRead(ec, bytes); }));
}

void Session::OnRead(const boost::system::error_code& ec, std::size_t bytes) {
    LOG_DEBUG("On Read bytes: {}", bytes);
    if (ec) {
        DoClose();
        return;
    }

    _last_active.store(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now().time_since_epoch())
            .count());

    LOG_INFO("On Read bytes: {}", bytes);
    _recv_buffer.insert(
        _recv_buffer.end(), _read_buf.begin(), _read_buf.begin() + bytes);
    ParsePackets();
    DoRead();
}

void Session::DoWrite() {
    auto self = shared_from_this();
    boost::asio::async_write(
        _socket,
        boost::asio::buffer(_write_queue.front()),
        boost::asio::bind_executor(_strand, [self](boost::system::error_code ec, std::size_t bytes) {
            self->OnWrite(ec, bytes);
        }));
}

void Session::OnWrite(const boost::system::error_code& ec, std::size_t) {
    if (ec) {
        DoClose();
        return;
    }

    _write_queue.pop_front();
    if (!_write_queue.empty()) {
        DoWrite();
    }
}


void Session::ParsePackets() {
    std::size_t read_pos  = 0;   // 当前读取到的位置
    std::size_t total_len = _recv_buffer.size();

    while (total_len - read_pos >= HEADER_LEN) {
        // 1. 预读取长度（不先删除，只是 peek）
        // 注意：这里使用 read_pos 作为偏移量
        uint32_t net_len;
        std::memcpy(&net_len, _recv_buffer.data() + read_pos, HEADER_LEN);
        LOG_DEBUG("recv body_len: {}", net_len);
        uint32_t body_len = ntohl(net_len);

        // 2. 安全检查
        if (body_len > MAX_BODY_LEN) {
            LOG_ERROR("[Session] Body too large: {}", body_len);
            DoClose();
            return;
        }

        // 3. 检查是否是一个完整的包 (Header + Body)
        // 如果 剩余数据 < 头部 + 包体，说明包不完整，停止解析，等待下次 OnRead
        if (total_len - read_pos < HEADER_LEN + HEADER_ID + body_len) {
            break;
        }

        // 提取msg_id
        uint16_t net_msg_id;
        std::memcpy(
            &net_msg_id,
            _recv_buffer.data() + read_pos + HEADER_LEN,
            HEADER_ID);
        uint16_t msg_id = ntohs(net_msg_id);
        LOG_INFO("[ChatServer] recv msg_id is: {}", msg_id);

        // 4. 提取包体
        // 此时我们确定数据是完整的，直接根据偏移量构造 string
        std::string body(
            _recv_buffer.begin() + read_pos + HEADER_LEN + HEADER_ID,
            _recv_buffer.begin() + read_pos + HEADER_LEN + HEADER_ID
                + body_len);
        LOG_INFO("recv body is: {}", body);

        // 5. 触发业务逻辑
        Message msg{msg_id, std::move(body)};
        OnMessage(msg);

        // 6. 移动读指针（跳过 Header + Body）
        read_pos += (HEADER_LEN + HEADER_ID + body_len);
    }

    // 7. 只有在所有能解的包都解完后，才进行一次内存移除
    if (read_pos > 0) {
        _recv_buffer.erase(
            _recv_buffer.begin(), _recv_buffer.begin() + read_pos);
    }

    // 重置期望长度（因为我们现在是每次重新解析 Header，不再依赖成员变量存状态）
    // _expected_len = 0;
}

void Session::OnMessage(const Message& msg) {
    _last_active.store(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now().time_since_epoch())
            .count());
    if (_dispatcher) {
        _dispatcher->Dispatch(shared_from_this(), msg);
    }
}

void Session::SetDispatcher(std::shared_ptr<Dispatcher> dispatcher) {
    _dispatcher = dispatcher;
}

const std::string& Session::Id() const {
    return _uuid;
}


void Session::SetCloseCallback(CloseCallback cb) {
    _on_close = std::move(cb);
}

Clock::time_point Session::LastActive() const {
    int64_t timestamp_ms = _last_active.load();
    return Clock::time_point(std::chrono::milliseconds(timestamp_ms));
}

void Session::PostClose() {
    auto self = shared_from_this();
    boost::asio::post(_strand, [self]() { self->DoClose(); });
}

void Session::SetUid(int uid) {
    _user_uid = uid;
}
