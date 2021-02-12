#ifndef _MarketRobot_Services_StageManager_H_
#define _MarketRobot_Services_StageManager_H_

#include "Common/config.h"
#include "Components/MrFactory.h"
#include "RegisterStages.h"
#include "StageBuilder.h"
#include "Stage.h"

namespace MarketRobot{

	class StageManager {
	public:
		static MR::Component::MrFactory<BROKERS, StageBuilder, Stage>
			s_stageFactory; /*!< Factory for creating Stages based off of the */

		static void init()
		{
			MarketRobot::register_stages();
		}
		/******************************************************************************/
		/*!
		Adds the type/StageBuilder pair to the stage factory.  Users should not need
		to call this function.  A batch file will automatically register all stages
		if they are named correctly: eg *Stage

		\param [in] type
		The stage type to assocaite the StageBuilder with

		\param [in] builder
		A pointer to a new StageBuilder type for this stage
		*/
		/******************************************************************************/
		static void StageManager::add_stage(BROKERS type, StageBuilder* builder)
		{
			s_stageFactory.add_builder(type, builder);
		}
		/******************************************************************************/
		/*!
		Removes a StageBuilder from the stage factory.

		\param [in] type
		The StageBuilder to remove based on the association when added.

		*/
		/******************************************************************************/
		static void remove_stage(BROKERS type)
		{
			s_stageFactory.remove_builder(type);
		};
		/******************************************************************************/
		/*!
		Clears all StageBuilders from the StageFactory
		*/
		/******************************************************************************/
		static void clear_stages(void)
		{
			s_stageFactory.clear_builders();
		}

		static Stage* build_stage(BROKERS type) {

			return s_stageFactory.build(type);

		}

	};

}

#endif // _MarketRobot_Services_StageManager_H_
