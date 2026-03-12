#include "grpcClient/AiChatClient.h"

int main(int argc, char *argv[]) {
    // 注意：当前接口是 pair<string, string&>，必须用可引用变量
    std::string                                        q1 = "你好";
    std::string                                        a1 = "你好，我是助手。";
    std::vector<std::pair<std::string, std::string &>> history
        = {{"human", q1}, {"assistant", a1}};

    auto rsp = AiChatClient::getInstance()->Chat(
        1001, "给我一句简短自我介绍", history);

    std::cout << "error: " << rsp.error() << "\n";
    std::cout << "error_msg: " << rsp.error_msg() << "\n";
    std::cout << "answer: " << rsp.answer() << "\n";

    // 方便脚本判断
    return rsp.error() == 0 ? 0 : 1;
}
