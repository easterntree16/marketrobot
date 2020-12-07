#ifndef __MarketRobot_Test_Log_H
#define __MarketRobot_Test_Log_H
#include <chrono>
#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <spdlog/spdlog.h>
#include "logger.h"
#include "mylogger.h"

using namespace MarketRobot;

#define DATE_TIME_FORMAT "%Y-%m-%d %H:%M:%S"

#define PRINT_TO_FILE_AND_CONSOLE(...) do{\
		logger::instance().Printf2File(__VA_ARGS__);\
		printf("%s ",ymdhms().c_str());printf(__VA_ARGS__);\
	}while (0)


#define PRINT_TO_FLUSH_AND_CONSOLE(...) do{\
		logger::instance().Printf2Flush(__VA_ARGS__);\
		printf("%s ",ymdhms().c_str());printf(__VA_ARGS__);\
	}while (0)

string ymdhms() {
	char buf[128] = { 0 };
	const size_t sz = sizeof("0000-00-00 00-00-00");
	{
		time_t timer;
		time(&timer);
		struct tm* tm_info = localtime(&timer);
		strftime(buf, sz, DATE_TIME_FORMAT, tm_info);
	}
	return string(buf);
}

void test(int _) {
	Logger::setup_log("test_logger");
	Logger::set_log_level(0);
	SPDLOG_TRACE("trace: hello, world!");
	SPDLOG_INFO("info: hello, world!");
}



#endif
