#ifndef __MarketRobot_COMPONENT_SPDLOG_H
#define __MarketRobot_COMPONENT_SPDLOG_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>


//#include "SystemDir.h"
#include "config.h"

#define DEFAULT_LOG_LEVEL spdlog::level::info
#define DEFAULT_LOG_PATTERN "[%Y-%m-%d %T.%F] [%^%=8l%$] [pid/tid %6P/%-6t] [%@#%!] %v"
//#define DEFAULT_LOG_PATTERN "[%Y-%m-%d %T.%F] [%^%=8l%$] [%@#%!] %v"


#define INSERT_ORDER_TRACE(content) SPDLOG_TRACE("[InsertOrder] {}", content)
#define INSERT_ORDER_ERROR(content) SPDLOG_ERROR("[InsertOrder] {}", content)
#define INSERT_ORDER_INFO(content)  SPDLOG_INFO("[InsertOrder] {}", content)

#define CANCEL_ORDER_TRACE(content) SPDLOG_TRACE("[CancelOrder] {}", content)
#define CANCEL_ORDER_ERROR(content) SPDLOG_ERROR("[CancelOrder] {}", content)
#define CANCEL_ORDER_INFO(content)  SPDLOG_INFO("[CancelOrder] {}", content)

#define ORDER_TRACE(content) SPDLOG_TRACE("[Order] {}", content)
#define ORDER_ERROR(content) SPDLOG_ERROR("[Order] {}", content)
#define ORDER_INFO(content)  SPDLOG_INFO("[Order] {}", content)

#define TRADE_TRACE(content) SPDLOG_TRACE("[Trade] {}", content)
#define TRADE_ERROR(content) SPDLOG_ERROR("[Trade] {}", content)
#define TRADE_INFO(content)  SPDLOG_INFO("[Trade] {}", content)

#define LOGIN_TRACE(content) SPDLOG_TRACE("[Login] {}", content)
#define LOGIN_INFO(content) SPDLOG_INFO("[Login] {}",content)
#define LOGIN_ERROR(content) SPDLOG_ERROR("[Login] {}", content)

#define CONNECT_INFO() SPDLOG_INFO("[Connected]")
#define DISCONNECTED_ERROR(content) SPDLOG_ERROR("[DisConnected] {}", content)

#define ACCOUNT_TRACE(content) SPDLOG_TRACE("[Account] {}", content)
#define ACCOUNT_ERROR(content)  SPDLOG_ERROR("[Account] {}", content)

#define POSITION_TRACE(content) SPDLOG_TRACE("[Position] {}", content)
#define POSITION_ERROR(content)  SPDLOG_ERROR("[Position] {}", content)

#define POSITION_DETAIL_TRACE(content) SPDLOG_TRACE("[PositionDetail] {}", content)
#define POSITION_DETAIL_ERROR(content)  SPDLOG_ERROR("[PositionDetail] {}", content)

#define INSTRUMENT_TRACE(content) SPDLOG_TRACE("[Instrument] {}", content)
#define INSTRUMENT_ERROR(content) SPDLOG_ERROR("[Instrument] {}", content)

#define SUBSCRIBE_TRACE(content) SPDLOG_TRACE("[Subscribe] {}" , content)
#define SUBSCRIBE_INFO(content) SPDLOG_INFO("[Subscribe] {}", content)
#define SUBSCRIBE_ERROR(content) SPDLOG_ERROR("[Subscribe] {}", content)

#define SUBSCRIBE_TRACE(content) SPDLOG_TRACE("[Subscribe] {}" , content)
#define SUBSCRIBE_INFO(content) SPDLOG_INFO("[Subscribe] {}", content)
#define SUBSCRIBE_ERROR(content) SPDLOG_ERROR("[Subscribe] {}", content)

#define UNSUBSCRIBE_TRACE(content) SPDLOG_TRACE("[Unsubscribe] {}" , content)
#define UNSUBSCRIBE_INFO(content) SPDLOG_INFO("[Unsubscribe] {}", content)
#define UNSUBSCRIBE_ERROR(content) SPDLOG_ERROR("[Unsubscribe] {}", content)

#define QUOTE_TRACE(content)  SPDLOG_TRACE("[Quote] {}", content)
#define ENTRUST_TRACE(content)  SPDLOG_TRACE("[Entrust] {}", content)
#define TRANSACTION_TRACE(content) SPDLOG_TRACE("[Transaction] {}", content)

#define TIMER_TRACE(content) SPDLOG_TRACE("[Timer] {}", content)

// spd 带行号的打印，同时输出console和文件
// #define DEBUG(...) SPDLOG_LOGGER_DEBUG(spdlog::default_logger_raw(), __VA_ARGS__);SPDLOG_LOGGER_DEBUG(spdlog::get("daily_logger"), __VA_ARGS__)
// #define LOG(...) SPDLOG_LOGGER_INFO(spdlog::default_logger_raw(), __VA_ARGS__);SPDLOG_LOGGER_INFO(spdlog::get("daily_logger"), __VA_ARGS__)
// #define WARN(...) SPDLOG_LOGGER_WARN(spdlog::default_logger_raw(), __VA_ARGS__);SPDLOG_LOGGER_WARN(spdlog::get("daily_logger"), __VA_ARGS__)
// #define ERROR(...) SPDLOG_LOGGER_ERROR(spdlog::default_logger_raw(), __VA_ARGS__);SPDLOG_LOGGER_ERROR(spdlog::get("daily_logger"), __VA_ARGS__)

#define DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG(...) SPDLOG_INFO(__VA_ARGS__)
#define WARN(...) SPDLOG_WARN( __VA_ARGS__)
#define ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define TRACE(...)  SPDLOG_TRACE(__VA_ARGS__)

class Logger {
public:
    static inline void setup_log(const std::string & name)
    {
		//SystemDir::create_folder_if_not_exists(SystemDir::getLogDir());
        std::string daily_log_path = MarketRobot::CConfig::instance().logDir() + name + ".log";
        std::cout << daily_log_path << std::endl;
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        // create log file by every day of 00：00
        auto daily_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(daily_log_path, 0, 0);
        // if warn flush logs，
        //daily_sink->flush_on(spdlog::level::warn);
        spdlog::sinks_init_list log_sinks = {console_sink, daily_sink};
        auto logger = std::make_shared<spdlog::logger>(name, log_sinks);
        logger->set_level(DEFAULT_LOG_LEVEL);
        logger->set_pattern(DEFAULT_LOG_PATTERN);
        spdlog::set_default_logger(logger);
    #ifdef _WINDOWS    
        spdlog::flush_every(std::chrono::seconds(1));
     #endif   
    }

    static inline void set_log_level(int level)
    {
        if (level < spdlog::level::trace || level > spdlog::level::off)
        {
            SPDLOG_ERROR("Invalid log level {}", level);
        }
        else
        {
            spdlog::default_logger()->set_level(static_cast<spdlog::level::level_enum>(level));
        }
    }

};


#endif //PROJECT_LOG_H
