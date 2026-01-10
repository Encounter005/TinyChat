#include "core/CServer.h"
#include "infra/ConfigManager.h"
#include "infra/LogManager.h"
#include <cstdlib>

int main() {
    auto        globalConfig  = ConfigManager::getInstance();
    std::string gate_port_str = (*globalConfig)["GateServer"]["port"];
    std::string gate_host_str = (*globalConfig)["GateServer"]["host"];
    // atoi: convert a string to an integer
    unsigned short gate_port = atoi(gate_port_str.c_str());

    try {
        net::io_context         ioc{1};   // 一个线程运行IO服务
        boost::asio::signal_set signals(
            ioc, SIGINT, SIGTERM);   // CTRL + C 发出关闭信号
        signals.async_wait(
            [&ioc](const boost::system::error_code& ec, int signal_number) {
                if (ec) {
                    return;
                }
                ioc.stop();
            });
        std::make_shared<CServer>(ioc, gate_port)->Start();
        ioc.run();

    } catch (std::exception& e) {
        LOG_ERROR("In GateServer main() exception is: {}", e.what());
        return EXIT_FAILURE;
    }


    return 0;
}
