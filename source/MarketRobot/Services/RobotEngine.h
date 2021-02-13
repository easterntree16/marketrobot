#ifndef _MarketRobot_Engine_RobotEngine_H_
#define _MarketRobot_Engine_RobotEngine_H_

#include "Common/config.h"
#include "Common/Data/marketdatafeed.h"
#include "Common/Brokerage/brokerage.h"
#include "DataCenter/datacenter.h"
#include "Services/Stage/Stage.h"
#include "Services/Stage/StageManager.h"

#include <thread>
#include <memory>

using std::vector;
using std::unique_ptr;

namespace MarketRobot
{

	class DLL_EXPORT_IMPORT RobotEngine {

		RUN_MODE mode = RUN_MODE::TRADE_MODE; //RUN_MODE::REPLAY_MODE; RUN_MODE::RECORD_MODE;
		BROKERS m_broker = BROKERS::GOOGLE;

		shared_ptr<marketdatafeed> pmkdata;
		shared_ptr<brokerage> pbrokerage;

		vector<unique_ptr<thread>> threads;

	public:
		void init() {
			// init StageManager to build the Map of broker->builder
			StageManager::init();

		};
		int run();
		bool live() const;

		RobotEngine();
		~RobotEngine();

		// https://mail.python.org/pipermail/cplusplus-sig/2004-July/007472.html
		// http://stackoverflow.com/questions/10142417/boostpython-compilation-fails-because-copy-constructor-is-private
		// For Boost::Python
		RobotEngine(RobotEngine&&) = delete;
		RobotEngine(const RobotEngine&) = delete;
		RobotEngine& operator=(RobotEngine&&) = delete;
		RobotEngine& operator=(const RobotEngine&) = delete;
	};
}

#endif // _MarketRobot_Engine_RobotEngine_H_


