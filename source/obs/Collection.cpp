/*
 * Plugin Includes
 */
#include "include/obs/Collection.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Collection::Collection(std::string name) : m_name(name) {
}

Collection::~Collection() {
}

CollectionManager::CollectionManager() :
	m_activeCollection(nullptr),
	m_isBuildingCollections(false) {
}

CollectionManager::~CollectionManager() {
}

/*
========================================================================================================
	Scenes Helpers
========================================================================================================
*/

void
Collection::buildScenes() {
	obs_frontend_set_current_scene_collection(m_name.c_str());

	obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);
	for(size_t i = 0; i < scenes.sources.num; i++) {

		Scene scene(this, scenes.sources.array[i]);
		Scene& scene_inserted = m_scenes
			.insert(std::map<std::string,Scene>::value_type(scene.name(), std::move(scene)))
			.first->second;

		scene_inserted.buildItems();
	}

	obs_frontend_source_list_free(&scenes);
}

/*
========================================================================================================
	Collections Management
========================================================================================================
*/

CollectionManager::obs_collection_event
CollectionManager::buildCollections() {
	obs_collection_event event = obs_collection_event::OBS_COLLECTIONS_LIST_BUILD;

	m_isBuildingCollections = true;

	char** collections = obs_frontend_get_scene_collections();
	char* active_collection = obs_frontend_get_current_scene_collection();
	m_activeCollection = nullptr;
	int i = 0;

	size_t old_count = m_collections.size();
	m_collections.clear();

	while(collections[i] != NULL) {
		bool active = strcmp(collections[i], active_collection) == 0;
		
		Collection collection(collections[i]);
		Collection& collection_inserted = m_collections
			.insert(std::map<std::string, Collection>::value_type(collections[i], collection))
			.first->second;

		if(active) m_activeCollection = &collection_inserted;

		collection_inserted.buildScenes();

		++i;
	}

	bfree(collections);
	
	size_t new_count = m_collections.size();

	if(old_count < new_count) {
		event = obs_collection_event::OBS_COLLECTION_EVENT_ADDED;
	}
	else if(old_count == new_count) {
		event = obs_collection_event::OBS_COLLECTION_EVENT_UPDATED;
	}
	else if(old_count > new_count) {
		event = obs_collection_event::OBS_COLLECTION_EVENT_REMOVED;
	}

	obs_frontend_set_current_scene_collection(active_collection);

	m_isBuildingCollections = false;

	return event;
}

Collection*
CollectionManager::getCollectionByName(const std::string& name) {
	Collection* collection = nullptr;
	if(m_collections.find(name) != m_collections.end()) {
		collection = &m_collections[name];
	}
	return collection;
}

/*
========================================================================================================
	Accessors
========================================================================================================
*/

std::string
Collection::name() const {
	return m_name;
}

std::string
Collection::id() const {
	return m_name;
}

Scenes
Collection::scenes() const {
	Scenes scenes;
	for(auto iter = m_scenes.begin(); iter != m_scenes.end(); iter++)
		scenes.push_back(const_cast<Scene*>(&(iter->second)));
	return scenes;
}

Collections
CollectionManager::collections() const {
	Collections collections;
	for(auto iter = m_collections.begin(); iter != m_collections.end(); iter++)
		collections.push_back(const_cast<Collection*>(&(iter->second)));
	return collections;
}

Collection*
CollectionManager::activeCollection() const {
	const char* current_collection = obs_frontend_get_current_scene_collection();
	if(m_activeCollection->name().compare(current_collection) != 0) {
		m_activeCollection = (Collection*)&m_collections.find(current_collection)->second;
	}
	return m_activeCollection;
}

bool
CollectionManager::isBuildingCollections() const {
	return m_isBuildingCollections;
}