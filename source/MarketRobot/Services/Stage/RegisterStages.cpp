
#include "RegisterStages.h"
#include "Common/config.h"
#include "Services/Stage/StageManager.h"
#include "Services/Stage/StageBuilder.h"
#include "Services/Stage/Stage.h"


namespace MarketRobot
{
	//		IB = 0, CTP, GOOGLE, SINA, PAPER, BTCC, OKCOIN
	void register_stages(void) {

		StageManager::add_stage(BROKERS::IB, new StageTBuilder< IbStage >());
		StageManager::add_stage(BROKERS::CTP, new StageTBuilder< CtpStage >());
		StageManager::add_stage(BROKERS::GOOGLE, new StageTBuilder< GoogleStage >());
		StageManager::add_stage(BROKERS::SINA, new StageTBuilder< SinaStage >());
		StageManager::add_stage(BROKERS::PAPER, new StageTBuilder< PaperStage >());
		StageManager::add_stage(BROKERS::BTCC, new StageTBuilder< BtccStage >());

	}

}
