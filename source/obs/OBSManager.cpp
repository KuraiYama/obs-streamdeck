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
	m_lastCollectionID(0x0) {
	m_isLoadingCollections = true;
	std::map<std::string, Collection*> collections = std::move(loadCollections());
	m_isLoadingCollections = false;
}

OBSManager::~OBSManager() {
	saveCollections();
}

OBSManager::FileLoader::FileLoader(const char* filename, std::ios_base::openmode mode) {
	try {
		if(open(filename, mode) == false) {
			m_stream.close();
			char message[1024];
			sprintf(message, "Error when loading file: %s", filename);
			throw std::exception(message);
		}
	}
	catch(std::exception& e) {
		throw std::exception(e.what());
	}
}

OBSManager::FileLoader::~FileLoader() {
	if(m_stream.is_open())
		m_stream.close();
}

/*
========================================================================================================
	File Handling
========================================================================================================
*/

bool
OBSManager::FileLoader::open(const char* filename, std::ios_base::openmode mode) {
	if(m_stream.is_open())
		m_stream.close();
	m_stream.open(filename, mode | std::fstream::binary);
	if(m_stream.good()) {
		m_stream.exceptions(std::fstream::failbit | std::fstream::badbit | std::fstream::eofbit);
	}

	return m_stream.is_open();
}

size_t
OBSManager::FileLoader::read(char* buffer, size_t size) {
	if(!m_stream.is_open() || !m_stream.good())
		return 0;

	try {
		m_stream.read(buffer, size);
	}
	catch(std::fstream::failure) {
		m_stream.close();
	}
	return m_stream.gcount();
}

size_t
OBSManager::FileLoader::write(char* buffer, size_t size) {
	if(!m_stream.is_open() || !m_stream.good())
		return 0;
	try {
		m_stream.write(buffer, size);
	}
	catch(std::fstream::failure) {
		m_stream.close();
		return 0;
	}

	return size;
}

/*
========================================================================================================
	Collections Management
========================================================================================================
*/

std::map<std::string, Collection*>
OBSManager::loadCollections() {

	std::map<std::string, Collection*> collections;

	try {
		FileLoader collections_file("collections.dat");

		// Read numbers of collections
		short collections_count = 0;
		collections_file.read((byte*)&collections_count, sizeof(short));

		while(collections_count > 0) {
			Collection* collection = nullptr;

			// Read block size
			size_t block_size = 0;
			collections_file.read((byte*)&block_size, sizeof(size_t));

			// Read block
			Memory block(block_size);
			collections_file.read(block, block_size);

			// Build collection (Threadable)
			collection = Collection::buildFromMemory(block);
			if(collection != nullptr) {
				collections[collection->name()] = collection;
				m_lastCollectionID = std::max<uint16_t>(m_lastCollectionID, collection->id());
			}
			collections_count--;
		}
	}
	catch(std::exception& e) {
		log_error << QString("OBS Manager failed on loading file - %1").arg(e.what()).toStdString();
	}

	return collections;
}

void
OBSManager::saveCollections() {
	std::vector<Memory> collection_blocks;

	/*BLOCK
		nbCollections (short)
		foreach(collection)
			block_size (size_t)
			block_collection (block_size)
	*/
	size_t size = sizeof(short);
	for(auto collection = m_collections.begin(); collection != m_collections.end(); collection++) {
		size += sizeof(size_t);
		collection_blocks.push_back(collection->second->toMemory(size));
	}

	short collections_count = m_collections.size();
	Memory block(size);

	// Write the number of collections
	block.write((byte*)&collections_count, sizeof(short));

	// For each collection
	for(auto iter = collection_blocks.begin(); iter < collection_blocks.end(); iter++) {
		size_t size_cl = iter->size();
		block.write((byte*)&size_cl, sizeof(size_t));
		block.write((*iter), iter->size());
	}

	try {
		FileLoader collections_file("collections.dat", std::ios::out);
		collections_file.write(block, block.size());
	}
	catch(std::exception& e) {
		log_error << QString("OBS Manager failed on writing file - %1").arg(e.what()).toStdString();
	}
}

void
OBSManager::extractFromOBSCollections(std::map<std::string, Collection*>& collections) {
	bool collections_preload = collections.size() > 0;

	char** obs_collections = obs_frontend_get_scene_collections();
	char* current_collection = obs_frontend_get_current_scene_collection();

	int i = 0;

	while(obs_collections[i] != NULL) {
		Collection* collection_loaded = nullptr;
		if(collections_preload) {
			auto collection_it = collections.find(obs_collections[i]);
			if(collection_it != collections.end()) {
				m_collections.insert(
					std::map<unsigned long long, std::shared_ptr<Collection>>::value_type(
						collection_it->second->id(),
						collection_it->second
					)
				);
				collection_loaded = collection_it->second;
				collections.erase(collection_it);
			}
		}

		if(collection_loaded == nullptr) {
			++m_lastCollectionID;
			collection_loaded = new Collection(m_lastCollectionID, obs_collections[i]);
			m_collections.insert(
				std::map<unsigned long long, std::shared_ptr<Collection>>::value_type(
					m_lastCollectionID,
					collection_loaded
				)
			);
		}

		loadScenes(*collection_loaded);
		++i;
	}

	bfree(obs_collections);

	obs_frontend_set_current_scene_collection(current_collection);
}

obs::collection_event
OBSManager::updateCollections(std::shared_ptr<Collection>& collection_updated) {
	obs::collection_event event = obs::collection_event::COLLECTIONS_LIST_BUILD;

	std::map<std::string, std::shared_ptr<Collection>> collections;
	for(auto iter = m_collections.begin(); iter != m_collections.end(); iter++) {
		collections[iter->second->name()] = iter->second;
	}

	char** obs_collections = obs_frontend_get_scene_collections();

	int i = 0, j = -1;
	while(obs_collections[i] != NULL) {
		auto iter = collections.find(obs_collections[i]);
		
		if(iter == collections.end()) {
			j = i;
		}
		else {
			collections.erase(iter);
		}
		++i;
	}

	if(j != -1) {
		// New collection
		if(collections.size() == 0) {
			++m_lastCollectionID;
			m_collections.insert(
				std::map<unsigned long long, std::shared_ptr<Collection>>::value_type(
					m_lastCollectionID,
					new Collection(m_lastCollectionID, obs_collections[j])
				)
			);
			collection_updated = m_collections.find(m_lastCollectionID)->second;
			loadScenes(*collection_updated);
			m_activeCollection = nullptr;
			event = obs::collection_event::COLLECTION_ADDED;
		}
		// Update collection
		else if(collections.size() == 1) {
			collections.begin()->second->name() = obs_collections[j];
			collection_updated = collections.begin()->second;
			event = obs::collection_event::COLLECTION_RENAMED;
		}
	}
	else if(collections.size() > 0) {
		collection_updated = collections.begin()->second;
		m_collections.erase(collections.begin()->second->id());
		m_activeCollection = nullptr;
		event = obs::collection_event::COLLECTION_REMOVED;
	}

	return event;
}

bool
OBSManager::switchCollection(unsigned long long id) {
	if(m_activeCollection->id() == id)
		return true;

	m_isLoadingCollections = true;

	auto iter = m_collections.find(id);
	if(iter != m_collections.end()) {
		obs_frontend_set_current_scene_collection(iter->second->name().c_str());
		const char* curr_collection = obs_frontend_get_current_scene_collection();
		if(iter->second->name().compare(curr_collection) == 0) {
			m_activeCollection = iter->second.get();
			m_isLoadingCollections = false;
			return true;
		}
	}
	m_isLoadingCollections = false;
	return false;
}

/*
========================================================================================================
	Scenes Management
========================================================================================================
*/

void
OBSManager::loadScenes(Collection& collection) {
	size_t size = m_lastCollectionID;
	collection.extractFromOBSScenes(size);
}

obs::scene_event
OBSManager::updateScenes(Collection& collection, std::shared_ptr<Scene>& scene_updated) {
	size_t size = m_lastCollectionID;
	return collection.updateScenes(size, scene_updated);
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
OBSManager::activeCollection(bool force_reset) const {
	if(force_reset) {
		const char* current_collection = obs_frontend_get_current_scene_collection();
		if(m_activeCollection) {
			if(m_activeCollection->name().compare(current_collection) == 0)
				return m_activeCollection;
			m_activeCollection = nullptr;
		}

		auto collection_it = m_collections.begin();
		while(collection_it != m_collections.end() && m_activeCollection == nullptr) {
			if(collection_it->second->name().compare(current_collection) == 0) {
				m_activeCollection = collection_it->second.get();
			}
			collection_it++;
		}
	}
	return m_activeCollection;
}

Collection*
OBSManager::collection(unsigned long long id) const {
	Collection* collection_ptr = nullptr;
	auto collection_it = m_collections.find(id);
	if(collection_it != m_collections.end())
		collection_ptr = collection_it->second.get();

	return collection_ptr;
}

bool
OBSManager::isLoadingCollections() const {
	return m_isLoadingCollections;
}