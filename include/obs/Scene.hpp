#pragma once

/*
 * Qt Includes
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

typedef std::vector<Scene*> Scenes;

class Scene {

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