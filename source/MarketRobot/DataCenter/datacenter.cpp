#include "DataCenter/datacenter.h"
#include "Common/Util/util.h"
#include "Common/Util/pair_hash.h"

#include <vector>

namespace MR::DC {
	using namespace MarketRobot;
	volatile sig_atomic_t DataCenter::signal_received_ = -1;
	DataCenter* DataCenter::pinstance_ = nullptr;
	mutex DataCenter::instancelock_;
	//void get_notified(int interval) {
	//	auto nano_str = time::strftime(time::now_in_nano());
	//	std::cout << "Observer|time up:" << nano_str << "|" << interval << std::endl;

	//}
	DataCenter::DataCenter() : quit_(true), running_(false), thread_(nullptr)
	{
		// message queue factory
		if (CConfig::instance()._msgq == MSGQ::ZMQ) {
			//msgq_pub_ = std::make_unique<CMsgqZmq>(MSGQ_PROTOCOL::PUB, CConfig::instance().BAR_AGGREGATOR_PUBSUB_PORT);
			msgq_pub_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::PUB, CConfig::instance().DATA_CENTER_PUBSUB_PORT);
		}
		else {
			msgq_pub_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::PUB, CConfig::instance().DATA_CENTER_PUBSUB_PORT);
		}

		// construct map for data storage
		start();
	}
	DataCenter::~DataCenter() {
		stop();
	}

	void DataCenter::signal_handler(int signal)
	{
		signal_received_ = signal;
	}

	void DataCenter::iteration()
	{
		// process the tick que every 1s
		std::this_thread::sleep_for(std::chrono::microseconds(1000000));
		{
			std::lock_guard lock_t(tick_queue_mutex_);
			if (!tick_queue_.empty()) {
				tick_swap_queue_.swap(tick_queue_);
			}
		}
		{
			std::lock_guard lock_b(bar_queue_mutex_);
			if (!bar_queue_.empty()) {
				bar_swap_queue_.swap(bar_queue_);
			}
		}
		if (!tick_swap_queue_.empty()) {
			DEBUG("Tick Swap size:{}",tick_swap_queue_.size());
		}
		while (!tick_swap_queue_.empty()) {
			Tick& tick = tick_swap_queue_.front();
			DEBUG("tick ={}", tick.str());
			tick_update_bar(tick);
			tick_swap_queue_.pop();
		}
		if (!bar_swap_queue_.empty()) {
			DEBUG("Bar Swap size:{}", bar_swap_queue_.size());
		}
		while (!bar_swap_queue_.empty()) {
			Bar* b = bar_swap_queue_.front();
			DEBUG("Bar ={}", b->str());
			onBar(b);
			bar_swap_queue_.pop();
		}
	}

	void DataCenter::start() {

		auto now_in_nano = time::now_in_nano();
		quit_ = false;

		for (auto& s : CConfig::instance().securities) {
			FullTick k;
			k.fullsymbol_ = s;
			latest_quotes_[s] = k;

			for (auto& t : time_intervals_) {
				auto time_interval = t * time_unit::NANOSECONDS_PER_SECOND;
				auto start_time = now_in_nano - now_in_nano % time_interval + time_interval;
				Bar bar(s, t);
				bar.start_time_ = start_time;
				bar.end_time_ = start_time + time_interval;

				BarSeries bs(s, t);
				bs.bars().push_back(bar);
				auto symbol_interval = std::make_pair(s, t);
				barseries_[symbol_interval] = bs;
			}
		}

		//create Frame timer to notify the time event
		timer_ptr_ = make_unique<FrameTimer>(time_intervals_);
		timer_ptr_->subscribe([&,this](int i) {this->onTime(i); });
		timer_ptr_->start();

		if (!running_)
		{
			running_ = true;
			thread_ = make_unique<std::thread>([&]() {
				run();
				});
		}

	}
	void DataCenter::run() {
		while (running_) {

			iteration();

		}
	}

	void DataCenter::stop()
	{
		bool stopping = false;

		if (running_)
		{
			quit_ = true;
			running_ = false;
		}
		thread_->join();
		clear();
	}
	void DataCenter::clear() {
		latest_quotes_.clear();
		barseries_.clear();
		securityDetails_.clear();
		
	}

	DataCenter& DataCenter::instance() {
		if (pinstance_ == nullptr) {
			lock_guard<mutex> g(instancelock_);
			if (pinstance_ == nullptr) {
				pinstance_ = new DataCenter();
			}
		}
		return *pinstance_;
	}
	void DataCenter::onTick(Tick& k) {

		if (k.datatype_ == DataType::DT_Bid) {
			latest_quotes_[k.fullsymbol_].bidprice_L1_ = k.price_;
			latest_quotes_[k.fullsymbol_].bidsize_L1_ = k.size_;
		}
		else if (k.datatype_ == DataType::DT_Ask) {
			latest_quotes_[k.fullsymbol_].askprice_L1_ = k.price_;
			latest_quotes_[k.fullsymbol_].asksize_L1_ = k.size_;
		}
		else if (k.datatype_ == DataType::DT_Trade) {
			latest_quotes_[k.fullsymbol_].price_ = k.price_;
			latest_quotes_[k.fullsymbol_].size_ = k.size_;
			//push tick into the tick que
			push_tick(k);
		}
		else if (k.datatype_ == DataType::DT_Full) {
			// default assigement shallow copy
			latest_quotes_[k.fullsymbol_] = dynamic_cast<FullTick&>(k);		

		}

	}
	void DataCenter::onBar(Bar* k) {

		auto & full_symbol = k->fullsymbol_;

		for (auto& t : time_intervals_) {
			auto symbol_interval = std::make_pair(full_symbol, t);
			if (barseries_.find(symbol_interval) != barseries_.end())
			{
				BarSeries& barseries = barseries_[symbol_interval];
				
				if (!barseries.bars().empty()) {
					Bar& current_bar = barseries.bars().back();
					current_bar.onBar(k);
				}
			}
		}
	}
	void DataCenter::tick_update_bar(const Tick& k) {

		auto& full_symbol = k.fullsymbol_;
		for (auto& t : time_intervals_) {
			auto symbol_interval = std::make_pair(full_symbol, t);
			if (barseries_.find(symbol_interval) != barseries_.end())
			{
				BarSeries& barseries = barseries_[symbol_interval];
				if (!barseries.bars().empty()) {
					Bar& current_bar = barseries.bars().back();
					current_bar.onTick(k);
				}
			}
		}

	}

	void DataCenter::onTime(int t) {
		auto now_in_nano = time::now_in_nano();
		for (auto& s : CConfig::instance().securities) {
			auto time_interval = t * time_unit::NANOSECONDS_PER_SECOND;
			auto start_time = now_in_nano - now_in_nano % time_interval;// +time_interval;
			Bar bar(s, t);
			bar.start_time_ = start_time;
			bar.end_time_ = start_time + time_interval;

			auto symbol_interval = std::make_pair(s, t);
			if (barseries_.find(symbol_interval) != barseries_.end())
			{
				BarSeries& barseries = barseries_[symbol_interval];
				if (!barseries.bars().empty()) {
					Bar& current_bar = barseries.bars().back();
					DEBUG("7:{}", current_bar.serialize());
					msgq_pub_->sendmsg(current_bar.serialize());
				}
				barseries.bars().push_back(bar);
			}

		}

	}

	void DataCenter::register_signal_callback(SignalCallback signal_callback)
	{
		signal_callbacks_.push_back(signal_callback);
	}

	void DataCenter::time_come() {
		auto now_in_nano = time::now_in_nano();
		for (auto& t : time_intervals_) {

		}

	}
	
	void DataCenter::push_bar(Bar* b) {
		if (b == nullptr || !b->isValid()) return;
		std::lock_guard lock(bar_queue_mutex_);
		bool is_notify = bar_queue_.empty();
		bar_queue_.push(b);
		//if (is_notify) bar_queue_cv_.notify_one();
	}

	void DataCenter::push_tick(Tick t) {
		std::lock_guard lock(tick_queue_mutex_);
		tick_queue_.push(t);

	}
}

