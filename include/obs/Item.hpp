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
#include "include/obs/OBSStorage.hpp"

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class Item;

class Scene;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

typedef Item* ItemPtr;

typedef struct Items {
	Scene* scene;
	std::vector<Item*> items;
} Items;

class Item : public OBSStorable {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		Scene* m_parentScene;

		obs_source_t* m_source;

		obs_sceneitem_t* m_item;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		Item(Scene* scene, uint16_t id, obs_sceneitem_t* item);

		Item(Scene* scene, uint16_t id, std::string name);

		virtual ~Item();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		virtual const char*
		type() const;

		obs_sceneitem_t*
		item() const;

		void
		item(obs_sceneitem_t* item);

		bool
		visible() const;

};