/******************************************************************************/
/*!
\file   MrFactory.h
\author Don Y
\par    email: donald_yangdong@yahoo.com
\par    Market Robot Engine
\date   2021/02/10

Templated class for creating  generic factory
*/
/******************************************************************************/
#ifndef _MarketRobot_Factory_H
#define _MarketRobot_Factory_H

#include <unordered_map>
namespace MR::Component{

	template<typename EnumType, typename BuilderType, typename ReturnType>
	class MrFactory
	{
	public:
		~MrFactory(void);
		bool add_builder(EnumType type, BuilderType* builder);
		void remove_builder(EnumType type);
		ReturnType* build(EnumType type);
		void clear_builders(void);
	private:
		//! Typedef for my Hash Table of Type and Builder's
		using BuilderMap = std::unordered_map<EnumType, BuilderType*>;

		//! Easy Typedef for the itorator to my BuilderMap.
		typedef typename BuilderMap::iterator    ArcheTypeItor;
		//! Container to map M5GameStages to M5Builders 
		BuilderMap m_builderMap;
	};

	/******************************************************************************/
	/*!
	Destructor makes sure to delete all builders from the factory
	*/
	/******************************************************************************/
	template<typename EnumType, typename BuilderType, typename ReturnType>
	MrFactory<EnumType, BuilderType, ReturnType>::~MrFactory(void)
	{
		clear_builders();
	}
	/******************************************************************************/
	/*!
	Adds a Builder to the Factory if it doesn't already exist.

	\param [in] type
	The type to associate with the given builder.

	\param [in] pBuilder
	A pointer to a Builder that will be owned and deleted by the factory.

	\return
	true if the insertion was succesful, false otherwise.
	*/
	/******************************************************************************/
	template<typename EnumType, typename BuilderType, typename ReturnType>
	bool MrFactory<EnumType, BuilderType, ReturnType>::add_builder(EnumType type, BuilderType* pBuilder)
	{
		std::pair<ArcheTypeItor, bool> itor = m_builderMap.insert(std::make_pair(type, pBuilder));
		return itor.second;
	}
	/******************************************************************************/
	/*!
	Removes a builder from the factory if it exists.

	\param [in] type
	The Type of the Enum/Builder to remove.
	*/
	/******************************************************************************/
	template<typename EnumType, typename BuilderType, typename ReturnType>
	void MrFactory<EnumType, BuilderType, ReturnType>::remove_builder(EnumType type)
	{
		BuilderMap::iterator itor = m_builderMap.find(type);
		if (itor == m_builderMap.end())
			return;

		//First delete the builder
		delete itor->second;
		itor->second = 0;
		//then erase the element
		m_builderMap.erase(itor);
	}
	/******************************************************************************/
	/*!
	Returns a new ReturnType pointer based on the type of the builder.

	\param [in] type
	The type of object to build

	\return
	A Derived class in the ReturnType inheritance chain or null if the builder
	didn't exist.
	*/
	/******************************************************************************/
	template<typename EnumType, typename BuilderType, typename ReturnType>
	ReturnType* MrFactory<EnumType, BuilderType, ReturnType>::build(EnumType type)
	{
		ArcheTypeItor itor = m_builderMap.find(type);
		return (itor == m_builderMap.end()) ? nullptr : itor->second->Build();
	}
	/******************************************************************************/
	/*!
	Removes all elements from the factory and deletes the associated builders.
	*/
	/******************************************************************************/
	template<typename EnumType, typename BuilderType, typename ReturnType>
	void MrFactory<EnumType, BuilderType, ReturnType>::clear_builders(void)
	{
		ArcheTypeItor itor = m_builderMap.begin();
		ArcheTypeItor end = m_builderMap.end();

		//Make sure to delete all builder pointers first
		while (itor != end)
		{
			delete itor->second;
			itor->second = 0;
			++itor;
		}

		//Then clear the hash table
		m_builderMap.clear();
	}

}
#endif //_MarketRobot_Factory_H
