#include "LogManager.h"
#include <iostream>
#include <spdlog/async.h>
#include <spdlog/common.h>


LogManager::LogManager() {
    try {
        // 8192 是队列中允许存放的消息最大数量
        // 1 是后台负责写日志的线程数量
        spdlog::init_thread_pool(8192, 1);
        // 1. 创建控制台输出槽 (彩色)
        auto console_sink
            = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);
        console_sink->set_color_mode(spdlog::color_mode::always);

        // 2. 创建循环文件输出槽 (单个文件最大 5MB, 最多保留 3 个文件)
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/server.log", 1024 * 1024 * 5, 3);
        file_sink->set_level(spdlog::level::info);

        // 3. 组合两个 Sinks
        _logger = std::make_shared<spdlog::async_logger>(
            "TinyChat",
            spdlog::sinks_init_list{console_sink, file_sink},
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block   // 队列满时的策略：阻塞或丢弃
        );

        spdlog::register_logger(_logger);

        // 4. 设置全局格式: [时间] [文件名:行号] [日志等级] 内容
        _logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s:%#] [%l] %v");

        // 设置默认等级为 Trace（宏会进一步根据编译配置过滤）
        _logger->set_level(spdlog::level::trace);

        // 设置每隔 3 秒自动刷新缓冲区到磁盘
        spdlog::flush_every(std::chrono::seconds(3));

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}
