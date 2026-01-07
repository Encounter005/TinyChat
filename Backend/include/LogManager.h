#ifndef LOGMANAGER_H_
#define LOGMANAGER_H_

#include "singleton.h"
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class LogManager : public SingleTon<LogManager> {
    friend class SingleTon<LogManager>;
public:
        ~LogManager() = default;
        auto get_logger() {
            return _logger;
        }
private:
    LogManager();
    std::shared_ptr<spdlog::logger> _logger;
};

#define LOG_TRACE(...) SPDLOG_LOGGER_CALL(LogManager::getInstance()->get_logger(), spdlog::level::trace, __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_CALL(LogManager::getInstance()->get_logger(), spdlog::level::debug, __VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_LOGGER_CALL(LogManager::getInstance()->get_logger(), spdlog::level::info, __VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_LOGGER_CALL(LogManager::getInstance()->get_logger(), spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_CALL(LogManager::getInstance()->get_logger(), spdlog::level::err, __VA_ARGS__)

#endif // LOGMANAGER_H_
