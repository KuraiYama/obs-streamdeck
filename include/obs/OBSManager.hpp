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
#include "include/obs/OBSEvents.hpp"
#include "include/obs/Collection.hpp"
#include "include/obs/Scene.hpp"
#include "include/obs/Item.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class OBSManager {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::map<std::string, Collection> m_collections;

		mutable Collection* m_activeCollection;

		bool m_isBuildingCollections;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		OBSManager();

		~OBSManager();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		obs::collection_event
		buildCollections();

		Collection*
		getCollectionByName(const std::string& name);

		Collections
		collections() const;

		Collection*
		activeCollection() const;

		bool
		isBuildingCollections() const;

};