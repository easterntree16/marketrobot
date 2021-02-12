#ifndef _MarketRobot_Services_StageBuilder_H_
#define _MarketRobot_Services_StageBuilder_H_


namespace MarketRobot{
	//Forward declaration
	class Stage;

	//! Base Builder class to create Game Stages via a StageFactory
	class StageBuilder
	{
	public:
		virtual ~StageBuilder() {} //empty virtual destructor
		//! Virtual Build call that must be overloaded by all Derived Builders
		virtual Stage* Build(void) = 0;
	};

	/*! Templated builder derived class so I don't need to create a Builder for each
	Stage type*/
	template <typename T>
	class StageTBuilder : public StageBuilder
	{
	public:
		virtual Stage* Build(void);
	};


	//! Creates a new Stage of type T
	template <typename T>
	Stage* StageTBuilder<T>::Build(void)
	{
		return new T();
	}


}
#endif // _MarketRobot_Services_StageBuilder_H_
