#ifndef HTTPCONNECTION_H_
#define HTTPCONNECTION_H_

#include "const.h"

class HttpConnection : public std::enable_shared_from_this<HttpConnection> {
public:
    friend class LogicSystem;
    explicit HttpConnection(tcp::socket socket);
    tcp::socket& GetSocket();
    // ~HttpConnection() = default;
    ~HttpConnection() { LOG_INFO("http connection closed"); }
    void Start();

private:
    void                               CheckDeadline();
    void                               WriteResponse();
    void                               HandleReq();
    void                               PreParseGetParam();
    unsigned char                      HextoDec(unsigned char);
    unsigned char                      ToHex(unsigned char);
    std::string                        UrlEncode(std::string_view);
    std::string                        UrlDecode(std::string_view);
    tcp::socket                        _socket;         // 上下文
    beast::flat_buffer                 _buffer{8192};   // 缓冲区
    http::request<http::dynamic_body>  _request;        // 请求
    http::response<http::dynamic_body> _response;       // 响应
    net::steady_timer                  _deadline{
        _socket.get_executor(), std::chrono::seconds(60)};   // 定时器
    std::string                        _get_url;
    std::map<std::string, std::string> _get_params;
};

#endif   // HTTPCONNECTION_H_
