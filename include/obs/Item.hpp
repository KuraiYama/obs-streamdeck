#pragma once

/*
 * Qt Includes
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
#include "include/obs/OBSStorage.hpp"

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class Item;

class Scene;

class Source;

class ItemBuilder;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

typedef Item* ItemPtr;

typedef struct Items {
	const Scene* scene;
	std::vector<ItemPtr> items;
} Items;

class Item : public OBSStorable {

	friend class ItemBuilder;

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	protected:

		Scene* m_parentScene;

		obs_source_t* m_source;

		obs_sceneitem_t* m_item;

		Source* m_sourceRef;

		bool m_visible;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		Item(Scene* scene, uint16_t id, obs_sceneitem_t* item);

		Item(Scene* scene, uint16_t id, const std::string& name);

		virtual ~Item();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		Scene*
		scene() const;

		const Source*
		source() const;

		virtual const char*
		type() const;

		obs_sceneitem_t*
		item() const;

		void
		item(obs_sceneitem_t* item);

		bool
		visible() const;

		void
		visible(bool toggle);

};