#pragma once

/*
 * STL Includes
 */
#include <map>
#include <vector>

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
//#include "include/obs/Item.hpp"

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

typedef std::vector<ScenePtr> Scenes;

class Scene {

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

		//std::map<int64_t, Item> m_items;

		unsigned long long m_identifier;

		std::string m_name;

		Collection* m_parentCollection;

		obs_scene_t* m_scene;

		obs_source_t* m_source;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		Scene(Collection* collection, unsigned long long id, std::string name);

		Scene(Collection* collection, unsigned long long id, obs_source_t* source);

		~Scene();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		Memory
		toMemory(size_t& size) const;

		/*void
		buildItems();*/

		std::string
		name() const;

		unsigned long long
		id() const;

		void
		source(obs_source_t* obs_source);

		obs_scene_t*
		scene() const;

		/*Items
		items() const;*/

};