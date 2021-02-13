#include "Services/RobotEngine.h"
#include "Common/config.h"
#include "Common/Util/util.h"
#include "Common/Logger/spdlogger.h"
#include "Common/Data/bargenerator.h"
#include "Common/Order/ordermanager.h"
#include "Common/Security/portfoliomanager.h"
#include "Services/Strategy/strategyservice.h"
#include "Services/Data/dataservice.h"
#include "Services/Brokerage/brokerageservice.h"
#include "Services/Api/apiservice.h"
#include "Services/Stage/StageManager.h"

#include <iostream>
#include <string>
#include <future>
#include <atomic>

namespace MarketRobot
{
	extern std::atomic<bool> gShutdown;
	extern atomic<uint64_t> MICRO_SERVICE_NUMBER;

	RobotEngine::RobotEngine() {

		SpdLogger::setup_log(CConfig::instance().logName());
		DataCenter::instance();
		OrderManager::instance();
		PortfolioManager::instance();

		// TODO: check if there is an MarketRobot instance running already
		m_broker = CConfig::instance().m_broker;
		threads.reserve(8);
		// init the StageManager
		init();
	}

	RobotEngine::~RobotEngine() {

		while ((pbrokerage && pbrokerage->isConnectedToBrokerage()) || (pmkdata && pmkdata->isConnectedToMarketDataFeed())) {
			msleep(100);
		}

		while (MICRO_SERVICE_NUMBER > 0) {
			msleep(100);
		}

		if (CConfig::instance()._msgq == MSGQ::NANOMSG)
			nn_term();
		else if (CConfig::instance()._msgq == MSGQ::ZMQ)
			;

		//printf("waiting for threads joined...\n");
		for (auto& t : threads) {
			if (t->joinable()) {
				t->join();
			}
		}

		INFO("Exit trading engine.");
		spdlog::drop_all();
	}

	bool RobotEngine::live() const {
		return gShutdown == true;
	}

	int RobotEngine::run() {
		if (gShutdown)
			return 1;
		try {
			auto fu1 = async(launch::async, check_gshutdown, true);

			if (mode == RUN_MODE::TRADE_MODE) {

				INFO("TRADE_MODE");

				// Factory produce the Stage for the Broker type specified in Configuration				
				auto pStage = std::unique_ptr<Stage>(StageManager::build_stage(m_broker));
				if( pStage ){
					pmkdata = pStage->MarketFeed();
					pbrokerage = pStage->Brokerage();
				}

				if (pmkdata && pbrokerage) {

					INFO("Stage built,Music Up ...!");

					//this_thread::sleep_for(std::chrono::milliseconds(1));
					threads.push_back(make_unique<thread>(BrokerageService, pbrokerage, 0));
					threads.push_back(make_unique<thread>(MarketDataService, pmkdata,
						CConfig::instance().ib_client_id++));

					threads.push_back(make_unique<thread>(TickRecordingService));
					threads.push_back(make_unique<thread>(BarRecordService));
				}

			}
			else if (mode == RUN_MODE::RECORD_MODE) {
				
				INFO("RECORD_MODE");

				// Factory produce the Stage for the Broker type specified in Configuration				
				auto pStage = std::unique_ptr<Stage>(StageManager::build_stage(m_broker));

				if (pStage) {
					pmkdata = pStage->MarketFeed();
				}
				if( pmkdata ){

					INFO("Stage built,Record the Music ...!");

					threads.push_back(make_unique<thread>(MarketDataService, pmkdata,
						CConfig::instance().ib_client_id++));
					threads.push_back(make_unique<thread>(TickRecordingService));
					threads.push_back(make_unique<thread>(BarRecordService));
				}
			}
			else if (mode == RUN_MODE::REPLAY_MODE) {
				
				INFO("REPLAY_MODE\n");

				threads.push_back(make_unique<thread>(TickReplayService, CConfig::instance().filetoreplay));
				pbrokerage = make_shared<paperbrokerage>();
				threads.push_back(make_unique<thread>(BrokerageService, pbrokerage, 0));
			}
			else {
				LOG_ERROR("EXIT:Mode { %d } doesn't exist.",  mode);
				return 1;
			}

			if (CConfig::instance()._msgq == MSGQ::NANOMSG) {
				threads.push_back(make_unique<thread>(ApiService));				// communicate with outside client monitor
			}
			// It seems that zmq interferes with interactive brokers
			//		triggering error code 509: Exception caught while reading socket - Resource temporarily unavailable
			//		so internally nanomsg is used.
			//		Zmq add a tick data relay service in ApiService class
			else if (CConfig::instance()._msgq == MSGQ::ZMQ) {
				threads.push_back(make_unique<thread>(ApiService));
			}

			threads.push_back(make_unique<thread>(DataBoardService));		// update databoard
			//threads.push_back(new thread(StrategyManagerService));

			fu1.get(); //block here
		}
		catch (exception& e) {
			std::cout << "Thanks for using MarketRobot. GoodBye: " <<  e.what() << std::endl;
		}
		catch (...) {
			std::cout << "MarketRobot terminated in error!" << std::endl;
		}
		return 0;
	}

	
}
