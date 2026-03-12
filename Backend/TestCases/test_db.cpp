#include "dao/MsgDAO.h"
#include "repository/MessagePersistenceRepository.h"
#include <iostream>
#include <json/value.h>



void test_message() {
    // auto res
    //     = MsgDAO::getInstance()->getRecentMessagesGroupedByFriend(1062, 7, 50);
    auto res = MessagePersistenceRepository::GetRecentMessagesWithCache(1062, 7,  50);
    if (res.IsOK()) {
        Json::Value root;
        auto        msgs = res.Value();
        // for (const auto& msg : msgs) {
        //     for (auto& message : msg.messages) {
        //         std::cout << message << std::endl;
        //     }
        // }
    }
}

int main(int argc, char* argv[]) {

    test_message();
    return 0;
}
