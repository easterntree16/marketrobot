#ifndef _MarketRobot_Component_Logger_H_
#define _MarketRobot_Component_Logger_H_

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <mutex>
#include <ctime>
#include <windows.h>



using std::string;
using std::mutex;



namespace MarketRobot
{
	class logger {
		static logger* pinstance_;
		static mutex instancelock_;

		FILE* logfile = nullptr;
		logger();
		~logger();

	public:
		static logger& instance();

		void Initialize();

		void Printf2File(const char *format, ...);
		void Printf2Flush(const char* format, ...);
	};
}
#endif
