#pragma once

/*
 * STL Includes
 */
#include <map>
#include <vector>
#include <memory>

/*
 * OBS Includes
 */
#include <obs.h>
#include <obs-frontend-api/obs-frontend-api.h>
#include <obs-module.h>

/*
 * Plugin Includes
 */
#include "include/common/Memory.hpp"
#include "include/obs/OBSStorage.hpp"
#include "include/obs/Item.hpp"

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class Scene;

class Collection;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

typedef Scene* ScenePtr;

typedef struct Scenes {
	const Collection* collection;
	std::vector<ScenePtr> scenes;
} Scenes;

class Scene : public OBSStorable {

	/*
	====================================================================================================
		Constants
	====================================================================================================
	*/
	public:

		static const unsigned int MAX_NAME_LENGTH = 99;

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	public:

		static Scene* buildFromMemory(Collection* collection, Memory& memory);

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		OBSStorage<Item> m_items;

		Collection* m_parentCollection;

		obs_scene_t* m_scene;

		obs_source_t* m_source;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		Scene(Collection* collection, uint16_t id, std::string name);

		Scene(Collection* collection, uint16_t id, obs_source_t* source);

		~Scene();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		Item*
		createItem(obs_sceneitem_t* item);

		std::shared_ptr<Item>
		deleteItem(Item* item);

		void
		loadItems();

		bool
		makeActive();

		void
		synchronize();

		Collection*
		collection() const;

		Items
		items() const;

		obs_scene_t*
		scene() const;

		obs_source_t*
		source() const;

		void
		source(obs_source_t* obs_source);

		Memory
		toMemory(size_t& size) const;

};