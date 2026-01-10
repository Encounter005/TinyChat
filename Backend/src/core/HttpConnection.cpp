#include "HttpConnection.h"
#include "infra/LogManager.h"
#include "LogicSystem.h"
#include <boost/beast/http/dynamic_body_fwd.hpp>

HttpConnection::HttpConnection(tcp::socket socket)
    : _socket(std::move(socket)) {}

tcp::socket& HttpConnection::GetSocket() {
    return _socket;
}


void HttpConnection::Start() {
    auto self = shared_from_this();
    http::async_read(
        _socket,
        _buffer,
        _request,
        [self](beast::error_code ec, std::size_t bytes_transfered) {
            try {
                if (ec) {
                    LOG_ERROR("http error is: {}", ec.what());
                    return;
                }
                boost::ignore_unused(bytes_transfered);
                self->HandleReq();
                self->CheckDeadline();

            } catch (std::exception& e) {
                LOG_ERROR("In HttpConnection::Start() exception is: {}", e.what());
            }
        });
}

void HttpConnection::HandleReq() {
    _response.version(_request.version());
    _response.keep_alive(false);

    if (_request.method() == http::verb::get) {
        PreParseGetParam();
        bool success = LogicSystem::getInstance()->HandleGet(
            _get_url, shared_from_this());
        if (!success) {
            _response.result(http::status::not_found);
            _response.set(http::field::connection, "text/plain");
            beast::ostream(_response.body()) << "url not found!\n";
            WriteResponse();
        } else {
            _response.result(http::status::ok);
            _response.set(http::field::server, "GateServer");
            WriteResponse();
        }
        return;
    } else if (_request.method() == http::verb::post) {
        bool success = LogicSystem::getInstance()->HandlePost(
            _request.target(), shared_from_this());

        if (!success) {
            _response.result(http::status::not_found);
            _response.set(http::field::connection, "text/plain");
            beast::ostream(_response.body()) << "url not found!\n";
            WriteResponse();
        } else {
            _response.result(http::status::ok);
            _response.set(http::field::server, "GateServer");
            WriteResponse();
        }
        return;
    }
}

void HttpConnection::WriteResponse() {
    auto self = shared_from_this();
    _response.content_length(_response.body().size());
    http::async_write(
        _socket,
        _response,
        [self](beast::error_code ec, std::size_t bytes_transfered) {
            self->_socket.shutdown(tcp::socket::shutdown_send, ec);
            self->_deadline.cancel();
        });
}
// @brief: 检测当前链接是否超时，如果超时就关闭链接
void HttpConnection::CheckDeadline() {

    auto self = shared_from_this();
    _deadline.async_wait([self](beast::error_code ec) {
        if (!ec) {
            // close socket to cancel any outstanding operation
            self->_socket.close(ec);
        }
    });
}


void HttpConnection::PreParseGetParam() {
    std::string_view url       = _request.target();
    auto             query_pos = url.find('?');
    if (query_pos == std::string_view::npos) {
        _get_url = std::string(url);
        return;
    }

    _get_url                      = std::string(url.substr(0, query_pos));
    std::string_view query_string = url.substr(query_pos + 1);
    while (!query_string.empty()) {
        // find next '&'
        std::size_t      next_amp = query_string.find('&');
        std::string_view pair
            = (next_amp == std::string_view::npos
                   ? query_string
                   : query_string.substr(0, next_amp));

        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string_view::npos) {
            auto key = pair.substr(0, eq_pos);
            auto val = pair.substr(eq_pos + 1);

            _get_params[UrlDecode(key)] = UrlDecode(val);
        }

        if (next_amp == std::string_view::npos) {
            break;
        } else {
            query_string.remove_prefix(next_amp + 1);
        }
    }
}


unsigned char HttpConnection::ToHex(unsigned char x) {
    return x > 9 ? x + 55 : x + 48;
}

unsigned char HttpConnection::HextoDec(unsigned char x) {
    unsigned char y;
    if (x >= 'A' && x <= 'Z')
        y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z')
        y = x - 'a' + 10;
    else if (x >= '0' && x <= '9')
        y = x - '0';
    else
        assert(0);
    return y;
}

std::string HttpConnection::UrlEncode(std::string_view str) {
    std::string strTemp;
    strTemp.reserve(str.length());
    for (size_t i = 0; i < str.length(); i++) {
        // 判断是否仅有数字和字母构成
        if (isalnum((unsigned char) str[i]) || (str[i] == '-')
            || (str[i] == '_') || (str[i] == '.') || (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ')   // 为空字符
            strTemp += "+";
        else {
            // 其他字符需要提前加%并且高四位和低四位分别转为16进制
            strTemp += '%';
            strTemp += ToHex((unsigned char) str[i] >> 4);
            strTemp += ToHex((unsigned char) str[i] & 0x0F);
        }
    }
    return strTemp;
}


std::string HttpConnection::UrlDecode(std::string_view str) {
    std::string strTemp;
    strTemp.reserve(str.length());
    for (size_t i = 0; i < str.length(); i++) {
        // 还原+为空
        if (str[i] == '+') strTemp += ' ';
        // 遇到%将后面的两个字符从16进制转为char再拼接
        else if (str[i] == '%') {
            assert(i + 2 < str.length());
            unsigned char high = HextoDec((unsigned char) str[++i]);
            unsigned char low  = HextoDec((unsigned char) str[++i]);
            strTemp += high * 16 + low;
        } else
            strTemp += str[i];
    }
    return strTemp;
}
