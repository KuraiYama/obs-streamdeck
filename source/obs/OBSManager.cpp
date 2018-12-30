/*
 * STL Includes
 */
#include <fstream>
#include <algorithm>

/*
 * Plugin Includes
 */
#include "include/common/Logger.hpp"
#include "include/obs/OBSManager.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

OBSManager::OBSManager() :
	m_activeCollection(nullptr),
	m_isLoadingCollection(false),
	m_lastCollectionID(0x0) {
}

OBSManager::~OBSManager() {
}

/*
========================================================================================================
	Events Management
========================================================================================================
*/

void
OBSManager::addEventHandler(
	const obs::frontend::event event,
	EventObserver<obs::frontend::event>* handler
) {
#ifdef USE_SCENE_BY_FRONTEND
	m_frontendEvent.addHandler(std::make_pair(event, handler));
	m_sceneitemEvent.EventTrigger<SceneItemEventTrigger, obs::frontend::event>::addHandler(
		std::make_pair(event, handler)
	);
#else
	m_frontendEvent.addHandler(std::make_pair(event, handler));
#endif;
}

void
OBSManager::addEventHandler(const obs::save::event event, EventObserver<obs::save::event>* handler) {
	m_saveEvent.addHandler(std::make_pair(event, handler));
}

void
OBSManager::addEventHandler(
	const obs::output::event event,
	EventObserver<obs::output::event>* handler
) {
	m_outputEvent.addHandler(std::make_pair(event, handler));
}

void
OBSManager::addEventHandler(const obs::scene::event event, EventObserver<obs::scene::event>* handler) {
	m_sceneEvent.addHandler(std::make_pair(event, handler));
}

void
OBSManager::addEventHandler(const obs::item::event event, EventObserver<obs::item::event>* handler) {
#ifdef USE_SCENE_BY_FRONTEND
	m_sceneitemEvent.EventTrigger<SceneItemEventTrigger, obs::item::event>::addHandler(
		std::make_pair(event, handler)
	);
#else
	m_sceneitemEvent.addHandler(std::make_pair(event, handler));
#endif
}

void
OBSManager::addEventHandler(const obs::source::event event, EventObserver<obs::source::event>* handler) {
	m_sourceEvent.addHandler(std::make_pair(event, handler));
}

void
OBSManager::registerAllSourcesScenes() {
	Sources sources = m_activeCollection->sources();
	for(auto iter = sources.sources.begin(); iter < sources.sources.end(); iter++)
		registerSource(*iter);

	Scenes scenes = m_activeCollection->scenes();
	for(auto iter = scenes.scenes.begin(); iter < scenes.scenes.end(); iter++)
		registerScene(*iter);
}

void
OBSManager::cleanRegisteredSourcesScenes() {
	m_sourceEvent.removeAll();
	m_sceneEvent.removeAll();
	m_sceneitemEvent.removeAll();
}

/*
========================================================================================================
	Outputs Management
========================================================================================================
*/

void
OBSManager::registerOutput(obs_output_t* output) {
	m_outputEvent.addOutput(output);
}

void
OBSManager::unregisterOutput(obs_output_t* output) {
	m_outputEvent.removeOutput(output);
}

/*
========================================================================================================
	Sources Management
========================================================================================================
*/

void
OBSManager::registerSource(const Source* source) {
	if(source->registrable())
		m_sourceEvent.addSource(source);
}

void
OBSManager::unregisterSource(const Source* source) {
	if(source->registrable())
		m_sourceEvent.removeSource(source);
}

/*
========================================================================================================
	Scenes Management
========================================================================================================
*/

void
OBSManager::registerScene(const Scene* scene) {
	m_sceneEvent.addScene(scene);
	m_sceneitemEvent.addScene(scene);
	Items items = scene->items();
	for(auto iter = items.items.begin(); iter < items.items.end(); iter++)
		m_sceneitemEvent.addItem(*iter);
}

void
OBSManager::unregisterScene(const Scene* scene) {
	m_sceneEvent.removeScene(scene->scene());
	m_sceneitemEvent.removeScene(scene->scene());
	Items items = scene->items();
	for(auto iter = items.items.begin(); iter < items.items.end(); iter++)
		m_sceneitemEvent.removeItem((*iter)->item());
}

/*
========================================================================================================
	Items Management
========================================================================================================
*/

void
OBSManager::registerItem(const Item* item) {
	m_sceneitemEvent.addItem(item);
}

void
OBSManager::unregisterItem(const Item* item) {
	m_sceneitemEvent.removeItem(item->item());
}

/*
========================================================================================================
	Collections Management
========================================================================================================
*/

void
OBSManager::resetCollection() {
	if(m_activeCollection != nullptr)
		m_activeCollection->active = false;
	m_activeCollection = nullptr;
}

void
OBSManager::makeActive() {
	char* current_collection = obs_frontend_get_current_scene_collection();
	if(current_collection == nullptr) return;
	m_activeCollection = m_collections[current_collection];
	if(m_activeCollection != nullptr)
		m_activeCollection->active = true;
}

void
OBSManager::loadCollections(OBSStorage<Collection>& collections, const uint16_t last_collection_id) {
	char* current_collection_bf = obs_frontend_get_current_scene_collection();

	m_isLoadingCollection = true;

	m_lastCollectionID = std::max<uint16_t>(m_lastCollectionID, last_collection_id);

	char** obs_collections = obs_frontend_get_scene_collections();

	unsigned int i = 0;

	while(obs_collections[i] != NULL) {
		std::shared_ptr<Collection> collection = collections.pop(obs_collections[i]);
		if(collection == nullptr) {
			m_lastCollectionID++;
			collection.reset(new Collection(m_lastCollectionID, obs_collections[i]));
		}

		m_collections.push(collection);

		switchCollection(collection.get());
		collection->loadSources();
		collection->loadScenes();
		++i;
	}

	bfree(obs_collections);
	m_isLoadingCollection = false;

	char* current_collection_af = obs_frontend_get_current_scene_collection();
	if(strcmp(current_collection_bf, current_collection_af) == 0) {
		this->makeActive();
		this->m_activeCollection->synchronize();
		this->m_activeCollection->makeActive();

		registerAllSourcesScenes();
	}
	else {
		this->switchCollection(current_collection_bf);
	}
}

obs::collection::event
OBSManager::updateCollections(std::shared_ptr<Collection>& collection_updated) {
	obs::collection::event event = obs::collection::event::LIST_BUILD;

	std::set<std::string> collections;
	for(auto iter = m_collections.begin(); iter != m_collections.end(); iter++) {
		collections.insert(iter->second->name());
	}

	char** obs_collections = obs_frontend_get_scene_collections();

	unsigned int i = 0;
	int j = -1;

	while(obs_collections[i] != NULL) {
		auto iter = collections.find(obs_collections[i]);
		if(iter != collections.end()) {
			collections.erase(iter);
		}
		else {
			j = i;
		}
		i++;
	}

	if(j == -1) {
		collection_updated = m_collections.pop(*collections.begin());
		event = obs::collection::event::REMOVED;
	}
	else {
		const char* name = obs_collections[j];
		if(collections.size() == 0) {
			m_lastCollectionID++;
			collection_updated = std::shared_ptr<Collection>(new Collection(m_lastCollectionID, name));
			m_collections.push(collection_updated);
			collection_updated->switching = true;
			collection_updated->loadSources();
			collection_updated->loadScenes();
			this->makeActive();
			collection_updated->synchronize();
			event = obs::collection::event::ADDED;
		}
		else {
			collection_updated = m_collections.move(*collections.begin(), name);
			event = obs::collection::event::RENAMED;
		}
	}

	bfree(obs_collections);

	return event;
}

bool
OBSManager::switchCollection(Collection* collection) {
	if(collection != nullptr)
		obs_frontend_set_current_scene_collection(collection->name().c_str());
	return collection != nullptr;
}

bool
OBSManager::switchCollection(uint16_t id) {
	return switchCollection(m_collections[id]);
}

bool
OBSManager::switchCollection(const char* name) {
	if(name == NULL) return false;
	return switchCollection(m_collections[name]);
}

/*
========================================================================================================
	Accessors
========================================================================================================
*/

Collections
OBSManager::collections() const {
	Collections collections;
	for(auto iter = m_collections.begin(); iter != m_collections.end(); iter++)
		collections.push_back(const_cast<Collection*>(iter->second.get()));
	return collections;
}

Collection*
OBSManager::activeCollection() const {
	return m_activeCollection;
}

Collection*
OBSManager::collection(uint16_t id) const {
	return m_collections[id];
}

bool
OBSManager::isLoadingCollection() const {
	return m_isLoadingCollection;
}