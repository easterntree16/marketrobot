#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <thread>
#include <iostream>
#include <spdlog/spdlog.h>
#include "logger.h"
#include "mylogger.h"
#include "test_log.h"


using namespace MarketRobot;
using namespace std::this_thread;
using std::thread;


double test_mt1(const int& iters)
{
	printf("TEST CASE: Test logger with Spdlog\n");

	Logger::setup_log("test_spdlog_logger");
	Logger::set_log_level(0);

	using std::chrono::high_resolution_clock;
	auto start = high_resolution_clock::now();
	int flag{ 0 };
	vector<thread> threads;
	for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
		thread t([&flag,&iters]() {
			while (flag < iters) {
				LOG("Hello logger: msg number {}", flag);
				flag += 1;
			}
		});
		threads.push_back(move(t));
	}
	for (auto & t :threads) {
		t.join();
	}
	auto delta = high_resolution_clock::now() - start;
	auto delta_d = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();
	printf("Elapsed: %0.2f secs %d/sec\n", delta_d, int(iters / delta_d));
	return delta_d;
}

double test_mt2(const int& iters)
{
	printf("TEST CASE: Test in-house logger\n");

	//int iters = 50000;
	using std::chrono::high_resolution_clock;
	auto start = high_resolution_clock::now();
	int flag{ 0 };
	vector<thread> threads;
	for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
		thread t([&]() {
			while (flag < iters) {
				PRINT_TO_FILE_AND_CONSOLE(" [  info  ] [E:\Dev\MarketRobot\test\test_log.cpp:33#test1]Hello logger: msg number %d at %d \n",i,get_id());
				flag += 1;
			}
			});
		threads.push_back(move(t));
	}
	for (auto& t : threads) {
		t.join();
	}

	auto delta = high_resolution_clock::now() - start;
	auto delta_d = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();
	printf("Elapsed: %0.2f secs %d/sec\n", delta_d, int(iters / delta_d));
	return delta_d;
}

double test_mt3(const int& iters)
{
	printf("TEST CASE: Test in-house Flush logger\n");

	//int iters = 50000;
	using std::chrono::high_resolution_clock;
	auto start = high_resolution_clock::now();
	
	int flag{ 0 };
	vector<thread> threads;
	for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
		thread t([&]() {
			while (flag < iters) {
				PRINT_TO_FLUSH_AND_CONSOLE(" [  info  ] [E:\Dev\MarketRobot\test\test_log.cpp:33#test1]Hello logger: msg number %d at %d \n", i, get_id());
				flag += 1;
			}
			});
		threads.push_back(move(t));
	}
	for (auto& t : threads) {
		t.join();
	}

	auto delta = high_resolution_clock::now() - start;
	auto delta_d = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();
	printf("Elapsed: %0.2f secs %d/sec\n", delta_d, int(iters / delta_d));
	return delta_d;
}

int main() {
	vector<double> rets;
	int iters = 50000;
	rets.push_back(test_mt1(iters)); // spdlog
	rets.push_back(test_mt2(iters)); // in-house fwrite
	rets.push_back(test_mt3(iters)); // in-house fflush

	printf("===== multi-threads SpdLog vs. in-house Logger performance PK results =======\n");
	for (auto& ret : rets) {
		printf("Elapsed: %0.2f secs %d/sec\n", ret, int(iters / ret));
	}

}