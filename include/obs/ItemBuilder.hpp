#pragma once

/*
 * Plugin Includes
 */
#include "include/obs/Item.hpp"
#include "include/obs/ItemScene.hpp"
#include "include/obs/ItemGroup.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class ItemBuilder {

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	public:

		static ItemBuilder*
		instance() {
			static ItemBuilder instance;
			return &instance;
		}

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::map<std::string, Item*(ItemBuilder::*)(Scene*, uint16_t, obs_sceneitem_t*)> m_builders;

	/*
	====================================================================================================
		Constructors / Destructors
	====================================================================================================
	*/
	private:

		ItemBuilder() {
			m_builders["scene"] = &ItemBuilder::buildItemScene;
			m_builders["group"] = &ItemBuilder::buildItemGroup;
			m_builders["default"] = &ItemBuilder::buildItem;
		}

		ItemBuilder(ItemBuilder&&) = delete;

		ItemBuilder(ItemBuilder&) = delete;

		~ItemBuilder() = default;

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		Item*
		build(Scene* scene, obs_sceneitem_t* obs_item) {
			uint16_t id = static_cast<uint16_t>(obs_sceneitem_get_id(obs_item));
			obs_source_t* obs_source = obs_sceneitem_get_source(obs_item);
			const char* name = obs_source_get_name(obs_source);

			Item* item = nullptr;

			auto builder = m_builders.find(obs_source_get_id(obs_source));
			if(builder == m_builders.end())
				item = (this->*m_builders["default"])(scene, id, obs_item);
			else
				item = (this->*builder->second)(scene, id, obs_item);

			item->item(obs_item);

			return item;
		}

	private:

		Item*
		buildItem(Scene* scene, uint16_t id, obs_sceneitem_t* obs_item) {
			Item* item = new Item(scene, id, obs_item);
			scene->collection()->getSourceByName(item->name())->addReference(&item->m_sourceRef);
			return item;
		}

		Item*
		buildItemScene(Scene* scene, uint16_t id, obs_sceneitem_t* obs_item) {
			ItemScene* item = new ItemScene(scene, id, obs_item);
			scene->collection()->getSceneByName(item->name())->sourcedScene()
				.addReference(&item->m_sourceRef);
			return item;
		}

		Item*
		buildItemGroup(Scene* scene, uint16_t id, obs_sceneitem_t* obs_item) {
			ItemGroup* item = new ItemGroup(scene, id, obs_item);
			scene->collection()->getSourceByName(item->name())->addReference(&item->m_sourceRef);
			return item;
		}

	/*
	====================================================================================================
		Operators
	====================================================================================================
	*/
	private:

		ItemBuilder
		operator=(const ItemBuilder&) = delete;

		ItemBuilder&
		operator=(ItemBuilder&&) = delete;

};