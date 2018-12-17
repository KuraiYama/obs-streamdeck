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

		std::map<unsigned long long, Collection> m_collections;

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

	private:

		void
		extractFromOBSCollections(std::map<std::string, Collection>& collections);

};