#include <cassert>
#include <iostream>
#include "RedisManager.h"

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

    TestRedis();

    return 0;
}
