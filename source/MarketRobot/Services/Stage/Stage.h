#ifndef _MarketRobot_Services_Stage_H_
#define _MarketRobot_Services_Stage_H_

#include <Common/Data/marketdatafeed.h>
#include <Common/Brokerage/brokerage.h>
#include <Brokers/IB981/ibbrokerage.h>
#include <Brokers/Ctp/ctpdatafeed.h>
#include <Brokers/Ctp/ctpbrokerage.h>
#include <Brokers/Sina/sinadatafeed.h>
#include <Brokers/BtcChina/btcchinadatafeed.h>
#include <Brokers/OkCoin/okcoindatafeed.h>
#include <Brokers/Google/googledatafeed.h>
#include <Brokers/Paper/paperdatafeed.h>
#include <Brokers/Paper/paperbrokerage.h>

namespace MarketRobot {

	class Stage {
	public:
		// market data connection
		shared_ptr<marketdatafeed> pmkdata;

		// brokerage connection
		shared_ptr<brokerage> pbrokerage;

		shared_ptr<marketdatafeed> MarketFeed() {
			return pmkdata;
		}

		shared_ptr<brokerage> Brokerage() {
			return pbrokerage;
		}
	};

	// InteractiveBrokers
	class IbStage : public Stage {
	public:
		IbStage() {
			std::shared_ptr<IBBrokerage> tmp = std::make_shared<IBBrokerage>();
			pmkdata = tmp;
			pbrokerage = tmp;
		}
	};

	// China Futures Ctp connection
	class CtpStage : public Stage {
	public:
		CtpStage() {
			pmkdata = make_shared<ctpdatafeed>();
			pbrokerage = make_shared<ctpbrokerage>();
		}
	};
	// SINA web market Data feed
	class SinaStage : public Stage {
	public:
		SinaStage() {
			pmkdata = make_shared<sinadatafeed>();
			pbrokerage = make_shared<paperbrokerage>();
		}
	};

	// Google web Market Data Feed
	class GoogleStage : public Stage {
	public:
		GoogleStage() {
			pmkdata = make_shared<googledatafeed>();
			pbrokerage = make_shared<paperbrokerage>();
		}
	};

	// Paper simulation connection
	class PaperStage : public Stage {
	public:
		PaperStage() {
			pmkdata = make_shared<sinadatafeed>();
			pbrokerage = make_shared<paperbrokerage>();
		}
	};

	// BTCC crypto currency connection
	class BtccStage : public Stage {
	public:
		BtccStage() {
			pmkdata = make_shared<btcchinadatafeed>();
			pbrokerage = make_shared<paperbrokerage>();
		}
	};

	// BTCC crypto currency connection
	class OkcoinStage : public Stage {
	public:
		OkcoinStage() {
			pmkdata = make_shared<okcoindatafeed>();
			pbrokerage = make_shared<paperbrokerage>();
		}
	};

}

#endif // _MarketRobot_Services_Stage_H_
