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
#include "include/obs/OBSEvents.hpp"
#include "include/obs/Collection.hpp"
//#include "include/obs/Scene.hpp"
//#include "include/obs/Item.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class OBSManager {

	/*
	====================================================================================================
		Static Class Attributes
	====================================================================================================
	*/
	private:

		static unsigned long long _last_registered_id;

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::map<unsigned long long, std::shared_ptr<Collection>> m_collections;

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
		loadCollections();

		void
		saveCollections();

		obs::collection_event
		updateCollections(std::shared_ptr<Collection>& collection_updated);

		Collections
		collections() const;

	private:

		void
		extractFromOBSCollections(std::map<std::string, std::shared_ptr<Collection>>& collections);

};