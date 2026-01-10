#include <cassert>
#include <iostream>
#include "infra/RedisManager.h"
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include "infra/LogManager.h"

// 示例连接代码
void mysqlConnect() {
    try {
        sql::mysql::MySQL_Driver *driver;
        sql::Connection *con;

        driver = sql::mysql::get_mysql_driver_instance();
        // 从你的 ConfigManager 获取参数
        con = driver->connect("tcp://127.0.0.1:3306", "encounter005", "cxy");

        con->setSchema("TinyChat");
        LOG_INFO("MySQL connected successfully.");

        delete con; // 实际建议放入连接池维护，不要直接 delete
    } catch (sql::SQLException &e) {
        LOG_ERROR("MySQL Connect Error: {}", e.what());
    }
}
void TestRedis() {
    std::string value = "raven005";
    assert(RedisManager::getInstance()->Set("blogwebsite", value));
    assert(RedisManager::getInstance()->Get("blogwebsite", value) );
    assert(RedisManager::getInstance()->Get("nonekey", value) == false);
    assert(RedisManager::getInstance()->HSet("bloginfo","blogwebsite", "llfc.club"));
    assert(RedisManager::getInstance()->HGet("bloginfo","blogwebsite") != "");
    assert(RedisManager::getInstance()->ExistsKey("bloginfo"));
    assert(RedisManager::getInstance()->Del("bloginfo"));
    assert(RedisManager::getInstance()->ExistsKey("bloginfo") == false);
    assert(RedisManager::getInstance()->LPush("lpushkey1", value));
    assert(RedisManager::getInstance()->LPush("lpushkey1", value));
    assert(RedisManager::getInstance()->LPush("lpushkey1", value));
    assert(RedisManager::getInstance()->RPop("lpushkey1", value));
    assert(RedisManager::getInstance()->RPop("lpushkey1", value));
    assert(RedisManager::getInstance()->LPop("lpushkey1", value));
    assert(RedisManager::getInstance()->LPop("lpushkey2", value)==false);
    RedisManager::getInstance()->Close();
}

int main(int argc, char* argv[]) {
    mysqlConnect();

    return 0;
}
