#pragma once

/*
 * STL Includes
 */
#include <fstream>
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
#include "include/obs/OBSStorage.h"
#include "include/obs/OBSEvents.hpp"
#include "include/obs/Collection.hpp"

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

		OBSStorage<Collection> m_collections;

		mutable Collection* m_activeCollection;

		uint16_t m_lastCollectionID;

		bool m_isLoadingCollection;

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

		void
		loadCollections(OBSStorage<Collection>& collections, const uint16_t last_collection_id);

		bool
		isLoadingCollection() const;

		obs::collection_event
		updateCollections(std::shared_ptr<Collection>& collection_updated);

		void
		makeActive();

		bool
		switchCollection(Collection* collection);

		bool
		switchCollection(uint16_t id);

		bool
		switchCollection(const char* name);

		Collection*
		activeCollection() const;

		Collection*
		collection(uint16_t id) const;

		Collections
		collections() const;

		obs::scene_event
		updateScenes(Collection& collection, std::shared_ptr<Scene>& scene_updated);

};