#include "StageManager.h"

namespace MarketRobot {

	MR::Component::MrFactory<BROKERS, StageBuilder, Stage>
		StageManager::s_stageFactory;

}