

#include <filesystem>

#include "mylogger.h"
#include "config.h"

namespace fs = std::filesystem;
using std::lock_guard;

namespace MarketRobot
{

#define DATE_FORMAT "%Y-%m-%d"

	string ymd() {
		char buf[128] = { 0 };
		const size_t sz = sizeof("0000-00-00");
		{
			time_t timer;
			struct tm* tm_info;
			time(&timer);
			tm_info = localtime(&timer);
			strftime(buf, sz, DATE_FORMAT, tm_info);
		}
		return string(buf);
	}

	string nowMS() {
		char buf[128] = {};
#ifdef __linux__
		struct timespec ts = { 0,0 };
		struct tm tm = {};
		char timbuf[64] = {};
		clock_gettime(CLOCK_REALTIME, &ts);
		time_t tim = ts.tv_sec;
		localtime_r(&tim, &tm);
		strftime(timbuf, sizeof(timbuf), "%F %T", &tm);
		snprintf(buf, 128, "%s.%03d", timbuf, (int)(ts.tv_nsec / 1000000));
#else
		SYSTEMTIME SystemTime;
		GetLocalTime(&SystemTime);
		sprintf(buf, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
			SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,
			SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds);
#endif
		return buf;
	}
	logger* logger::pinstance_ = nullptr;
	mutex logger::instancelock_;

	logger::logger() : logfile(nullptr) {
		Initialize();
	}

	logger::~logger() {
		fclose(logfile);
	}

	logger& logger::instance() {
		if (pinstance_ == nullptr) {
			lock_guard<mutex> g(instancelock_);
			if (pinstance_ == nullptr) {
				pinstance_ = new logger();
			}
		}
		return *pinstance_;
	}

	void logger::Initialize() {
		string fname;
		if (CConfig::instance()._mode == RUN_MODE::REPLAY_MODE) {
			fname = CConfig::instance().logDir() + "marketrobot-replay-" + ymd() + ".txt";
		}
		else {
			fname = CConfig::instance().logDir() + "marketrobot-" + ymd() + ".txt";
		}

		logfile = fopen(fname.c_str(), "w");
		setvbuf(logfile, nullptr, _IONBF, 0);
	}

	void logger::Printf2File(const char *format, ...) {
		lock_guard<mutex> g(instancelock_);

		static char buf[1024 * 2];
		string tmp = nowMS();
		size_t sz = tmp.size();
		strcpy(buf, tmp.c_str());
		buf[sz] = ' ';

		va_list args;
		va_start(args, format);
		vsnprintf(buf + sz + 1, 1024 * 2 - sz - 1, format, args);
		size_t buflen = strlen(buf);
		fwrite(buf, sizeof(char), buflen, logfile);
		va_end(args);
	}

	void logger::Printf2Flush(const char* format, ...) {
		lock_guard<mutex> g(instancelock_);

		static char buf[1024 * 2];
		string tmp = nowMS();
		size_t sz = tmp.size();
		strcpy(buf, tmp.c_str());
		buf[sz] = ' ';

		va_list args;
		va_start(args, format);
		vsnprintf(buf + sz + 1, 1024 * 2 - sz - 1, format, args);
		size_t buflen = strlen(buf);
		fwrite(buf, sizeof(char), buflen, logfile);
		fflush(logfile);
		va_end(args);
	}

}
