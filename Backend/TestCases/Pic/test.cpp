#include "infra/LogManager.h"
#include "infra/RedisManager.h"
#include "service/UserService.h"
#include <cassert>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/statement.h>
#include <cstdint>
#include <iostream>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <netinet/in.h>

// 示例连接代码
void mysqlConnect() {
    try {
        sql::mysql::MySQL_Driver* driver;
        sql::Connection*          con;

        driver = sql::mysql::get_mysql_driver_instance();
        // 从你的 ConfigManager 获取参数
        con = driver->connect("tcp://127.0.0.1:3306", "encounter005", "cxy");

        con->setSchema("TinyChat");
        LOG_INFO("MySQL connected successfully.");

        delete con;   // 实际建议放入连接池维护，不要直接 delete
    } catch (sql::SQLException& e) {
        LOG_ERROR("MySQL Connect Error: {}", e.what());
    }
}
void TestRedis() {
    std::string value = "raven005";
    assert(RedisManager::getInstance()->Set("blogwebsite", value));
    assert(RedisManager::getInstance()->Get("blogwebsite", value));
    assert(RedisManager::getInstance()->Get("nonekey", value) == false);
    assert(
        RedisManager::getInstance()->HSet(
            "bloginfo", "blogwebsite", "llfc.club"));
    assert(RedisManager::getInstance()->HGet("bloginfo", "blogwebsite") != "");
    assert(RedisManager::getInstance()->ExistsKey("bloginfo"));
    assert(RedisManager::getInstance()->Del("bloginfo"));
    assert(RedisManager::getInstance()->ExistsKey("bloginfo") == false);
    assert(RedisManager::getInstance()->LPush("lpushkey1", value));
    assert(RedisManager::getInstance()->LPush("lpushkey1", value));
    assert(RedisManager::getInstance()->LPush("lpushkey1", value));
    assert(RedisManager::getInstance()->RPop("lpushkey1", value));
    assert(RedisManager::getInstance()->RPop("lpushkey1", value));
    assert(RedisManager::getInstance()->LPop("lpushkey1", value));
    assert(RedisManager::getInstance()->LPop("lpushkey2", value) == false);
    RedisManager::getInstance()->Close();
}


void test_json() {
    Json::Value data;
    UserInfo    userInfo;
    userInfo.email = "15";
    userInfo.name  = "16";
    userInfo.back  = "17";
    userInfo.icon  = "18";
}
using boost::asio::ip::tcp;

// 必须与服务器保持一致 (建议改为 4)
static constexpr uint32_t HEADER_LEN = 6;

void SendMessage(tcp::socket& socket, const std::string& msg) {
    // 1. 准备包头 (长度)
    uint32_t len = static_cast<uint32_t>(msg.size());
    // 转换为网络字节序 (Big Endian)
    uint32_t net_len = htonl(len);

    std::vector<char> packet;
    packet.resize(HEADER_LEN + len);

    // 2. 拷贝长度头
    // 注意：这里我们假设 HEADER_LEN == 4，正好对应 uint32_t
    std::memcpy(packet.data(), &net_len, 4);
    uint16_t msg_id     = 1;
    uint16_t net_msg_id = htons(msg_id);

    std::cout << "net_msg id is: " << net_msg_id << std::endl
              << "net_len is:" << net_len << std::endl
              << "body is: " << msg << std::endl;


    std::memcpy(packet.data() + 4, &net_msg_id, 2);
    // 3. 拷贝消息体
    std::memcpy(packet.data() + HEADER_LEN, msg.data(), len);

    // 4. 发送数据
    boost::asio::write(socket, boost::asio::buffer(packet));
}

std::string ReadMessage(tcp::socket& socket) {
    // 1. 先读取头部 (HEADER_LEN 字节)
    std::vector<char> head_buf(HEADER_LEN);
    boost::asio::read(socket, boost::asio::buffer(head_buf));

    // 2. 解析长度
    uint32_t net_len = 0;
    std::memcpy(&net_len, head_buf.data(), 4);
    // 网络字节序转主机字节序
    uint32_t body_len   = ntohl(net_len);
    uint16_t net_msg_id = 0;
    std::memcpy(&net_msg_id, head_buf.data() + 4, 2);
    auto msg_id = ntohs(net_msg_id);

    std::cout << "[Client] Header parsed, body length: " << body_len
              << " msg_id is: " << msg_id << std::endl;

    if (body_len == 0) return "";

    // 3. 根据长度读取消息体
    std::vector<char> body_buf(body_len);
    boost::asio::read(socket, boost::asio::buffer(body_buf));

    return std::string(body_buf.begin(), body_buf.end());
}


int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: ChatClient <host> <port>\n";
            std::cerr << "Example: ./ChatClient 127.0.0.1 8080\n";
            return 1;
        }

        const char* server_ip   = argv[1];
        const char* server_port = argv[2];

        boost::asio::io_context ioc;

        // 解析端点
        tcp::resolver resolver(ioc);
        auto          endpoints = resolver.resolve(server_ip, server_port);

        // 创建并连接 Socket
        tcp::socket socket(ioc);
        boost::asio::connect(socket, endpoints);

        std::cout << "Connected to server " << server_ip << ":" << server_port
                  << std::endl;
        std::cout << "Type message and hit Enter (or 'quit' to exit):"
                  << std::endl;

        while (true) {
            std::string line;
            std::cout << "> ";
            std::getline(std::cin, line);

            if (line == "quit") {
                break;
            }

            if (line.empty()) continue;

            // 发送
            SendMessage(socket, line);

            // 接收回显
            std::string reply = ReadMessage(socket);
            std::cout << "< Server Reply: " << reply << std::endl;
        }

        socket.close();

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
