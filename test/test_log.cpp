#include <chrono>
#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <spdlog/spdlog.h>
#include "logger.h"
#include "mylogger.h"
#include "test_log.h"



 
double test1(const int& iters)
{
	printf("TEST CASE: Test logger with Spdlog\n");
	
    Logger::setup_log("test_spdlog_logger");
    Logger::set_log_level(0);
 
	//int iters = 50000;
	using std::chrono::high_resolution_clock;
	auto start = high_resolution_clock::now();
	for (int i = 0; i < iters; i++)
	{
		LOG("Hello logger: msg number {}", i);
	}
	auto delta = high_resolution_clock::now() - start;
	auto delta_d = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();
	printf("Elapsed: %0.2f secs %d/sec\n", delta_d, int(iters / delta_d));
	return delta_d;
}

double test2(const int & iters)
{
	printf("TEST CASE: Test in-house logger\n");

	//int iters = 50000;
	using std::chrono::high_resolution_clock;
	auto start = high_resolution_clock::now();
	for (int i = 0; i < iters; i++)
	{
		PRINT_TO_FILE_AND_CONSOLE(" xxxxxx] [  info  ] [E:\Dev\MarketRobot\test\test_log.cpp:33#test1]Hello logger: msg number %d\n", i);
	}
	auto delta = high_resolution_clock::now() - start;
	auto delta_d = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();
	printf("Elapsed: %0.2f secs %d/sec\n", delta_d, int(iters / delta_d));
	return delta_d;
}

double test3(const int& iters)
{
	printf("TEST CASE: Test in-house Flush logger\n");

	//int iters = 50000;
	using std::chrono::high_resolution_clock;
	auto start = high_resolution_clock::now();
	for (int i = 0; i < iters; i++)
	{
		PRINT_TO_FLUSH_AND_CONSOLE(" xxxxxx] [  info  ] [E:\Dev\MarketRobot\test\test_log.cpp:33#test1]Hello logger: msg number %d\n", i);
	}
	auto delta = high_resolution_clock::now() - start;
	auto delta_d = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();
	printf("Elapsed: %0.2f secs %d/sec\n", delta_d, int(iters / delta_d));
	return delta_d;
}

int main() {
	vector<double> rets;
	int iters = 50000;
	rets.push_back(test1(iters)); // spdlog
	rets.push_back(test2(iters)); // in-house fwrite
	rets.push_back(test3(iters)); // in-house fflush

	printf("===== SpdLog vs. in-house Logger performance PK results =======\n");
	for (auto& ret : rets) {
		printf("Elapsed: %0.2f secs %d/sec\n", ret, int(iters / ret));
	}

}