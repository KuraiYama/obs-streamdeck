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
#include "include/obs/Scene.hpp"

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class Collection;

class CollectionManager;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

typedef std::vector<Collection*> Collections;

class Collection {

	/*
	====================================================================================================
		Types Definitions
	====================================================================================================
	*/
	public:

		enum class obs_scene_event {
			OBS_SCENE_EVENT_ADDED = 0,
			OBS_SCENE_EVENT_REMOVED,
			OBS_SCENE_EVENT_SWITCHED,

			OBS_SCENES_LIST_BUILD,
		};

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::map<std::string, Scene> m_scenes;

		std::string m_name;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		Collection(std::string name = "");

		~Collection();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void buildScenes();

		std::string id() const;

		std::string name() const;

		Scenes scenes() const;

};

class CollectionManager {

	/*
	====================================================================================================
		Types Definitions
	====================================================================================================
	*/
	public:

		enum class obs_collection_event {
			OBS_COLLECTION_EVENT_ADDED = 0,
			OBS_COLLECTION_EVENT_REMOVED,
			OBS_COLLECTION_EVENT_UPDATED,
			OBS_COLLECTION_EVENT_SWITCHED,

			OBS_COLLECTIONS_LIST_BUILD,
		};

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

		CollectionManager();

		~CollectionManager();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		obs_collection_event buildCollections();

		Collection* getCollectionByName(const std::string& name);

		Collections collections() const;

		Collection* activeCollection() const;

		bool isBuildingCollections() const;

};