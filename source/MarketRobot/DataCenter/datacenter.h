#ifndef _MarketRobot_DataCenter_DataCenter_H_
#define _MarketRobot_DataCenter_DataCenter_H_

#include "Common/Data/datatype.h"
#include "Common/Data/tick.h"
#include "Common/Security/security.h"
#include "Common/Data/bar.h"
#include "Common/Data/barseries.h"
#include "Components/frame_timer.h"
#include "Common/Util/pair_hash.h"
#include "Common/Logger/spdlogger.h"


#include <string>
#include <sstream>
#include <map>
#include <regex>
#include <csignal>
#include <mutex>
#include <condition_variable>


#ifdef _WIN32
#include <nanomsg/src/nn.h>
#include <nanomsg/src/pubsub.h>
#else
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#endif

using std::string;

namespace MR::DC
{
	/// DataCenter
	/// 1. provide latest full tick price info  -- DataBoard Service
	/// 2. provide bar series		-- Bar Service
	using MarketRobot::CMsgq;
	using MarketRobot::Tick;
	using MarketRobot::Bar;
	using MarketRobot::BarSeries;
	using MarketRobot::FullTick;
	using MarketRobot::Security;
	using MR::Component::FrameTimer;


	using TickCallback = std::function<void(Tick& t)>;
	using SignalCallback = std::function<void(int sig)>;
	using TickPtr = std::unique_ptr<Tick>;

	class DataCenter {
	public:
		std::unique_ptr<CMsgq> msgq_pub_;
		static DataCenter* pinstance_;
		static mutex instancelock_;
		static DataCenter& instance();

		DataCenter();
		~DataCenter();
		void stop();
		void start();
		void run();
		void iteration();
		void clear();
		//void push_tick(const FullTick& k);
		void onTick(Tick& k);
		void onBar(Bar* k);
		void onTime(int t); // t means t seconds of interval 

		void register_signal_callback(SignalCallback handler);
		//producer push Bar into Bar Que
		void push_bar(Bar* b);
		void push_tick(Tick t);
	private:
		unique_ptr<FrameTimer> timer_ptr_;
		queue<int> time_queue_;
		bool quit_;
		//std::unique_ptr<TaskScheduler> scheduler_;
		static volatile std::sig_atomic_t signal_received_;
		vector<SignalCallback> signal_callbacks_;
		static void signal_handler(int signal);
		void time_come();
		void tick_update_bar(const Tick& tick);

		// securities configed in config file
		std::map<std::string, Security> securityDetails_;
		// predefined time intervals
		vector<int> time_intervals_ = { 60,180,900,3600 };

		//std::unordered_map<std::pair<string, int>, Bar, pair_hash> bars_;
		std::unordered_map<std::pair<string, int>, BarSeries, pair_hash> barseries_;
		std::map<string, FullTick> latest_quotes_;
		std::map<string, Bar> latest_bars_;


		std::mutex tick_queue_mutex_;
		std::condition_variable tick_queue_cv_;

		//Tick input Queue
		std::queue<Tick> tick_queue_;
		//Tick Cache Queue
		std::queue<Tick> tick_swap_queue_;

		std::mutex bar_queue_mutex_;
		std::condition_variable bar_queue_cv_;
		//5s Bar input Queue
		std::queue<Bar*> bar_queue_;
		//5s Bar Cache Queue
		std::queue<Bar*> bar_swap_queue_;
		unique_ptr<std::thread> thread_;
		bool running_;
	};
}
#endif // _MarketRobot_DataCenter_DataCenter_H_
