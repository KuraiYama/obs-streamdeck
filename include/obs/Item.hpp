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

typedef std::vector<Item*> Items;

class Item {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::string m_name;

		Scene* m_parentScene;

		obs_source_t* m_source;

		obs_sceneitem_t* m_sceneItem;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		Item(Scene* scene, obs_sceneitem_t* item);

		virtual ~Item();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		std::string name() const;

		std::string completeName() const;

		virtual const char* type() const;

		int64_t id() const;

		bool visible() const;

};