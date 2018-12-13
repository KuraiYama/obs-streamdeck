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

typedef std::vector<Scene*> Scenes;

class Scene {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::map<int64_t, Item> m_items;

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

		Scene(Collection* collection, obs_source_t* source);

		Scene(const Scene& scene);

		Scene(Scene&& scene);

		~Scene();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void buildItems();

		std::string name() const;

		std::string id() const;

		obs_scene_t* scene() const;

		Items items() const;

};