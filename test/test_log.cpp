#include <iostream>
#include <spdlog/spdlog.h>
#include "logger.h"


int main() {
    
	Logger::setup_log("test_logger");
	Logger::set_log_level(0);
    SPDLOG_TRACE("trace: hello, world!");
    SPDLOG_INFO("info: hello, world!");
    system("pause");
}