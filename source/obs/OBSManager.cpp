/*
 * STL Includes
 */
#include <fstream>
#include <algorithm>

/*
 * Plugin Includes
 */
#include "include/obs/OBSManager.hpp"

/*
========================================================================================================
	Static Class Attributes Initializations
========================================================================================================
*/

unsigned long long OBSManager::_last_registered_id = 0x0;

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

OBSManager::OBSManager() /*:
	m_activeCollection(nullptr),
	m_isBuildingCollections(false)*/ {
}

OBSManager::~OBSManager() {
	m_collections.clear();
}

/*
========================================================================================================
	Collections Management
========================================================================================================
*/

void
OBSManager::saveCollections() {
	std::ofstream collectionsFile;
	collectionsFile.open("collections.dat", std::ofstream::out | std::ofstream::binary);

	if(collectionsFile.good()) {

		// Write the number of registered collections
		short collections_count = m_collections.size();
		collectionsFile.write((char*)&collections_count, sizeof(short));

		if(collectionsFile) {

			collectionsFile.exceptions(
				std::ifstream::failbit |
				std::ifstream::badbit |
				std::ifstream::eofbit
			);

			char* buffer = nullptr;

			try {
				auto collection_it = m_collections.begin();
				while(collection_it != m_collections.end()) {

					size_t buff_size = collection_it->second->toBytes(&buffer);
					if(buff_size > 0) {
						collectionsFile.write((char*)&buff_size, sizeof(size_t));
						collectionsFile.write(buffer, buff_size);
						delete [] buffer;
						buffer = nullptr;
					}
					collection_it++;
				}
			}
			catch(std::ofstream::failure e) {
				if(buffer != nullptr) {
					delete [] buffer;
				}
			}
		}
	}

	collectionsFile.close();
}

void
OBSManager::loadCollections() {
	std::ifstream collectionsFile;
	collectionsFile.open("collections.dat", std::ifstream::in | std::ifstream::binary);

	std::map<std::string, std::shared_ptr<Collection>> collections_preload;

	if(collectionsFile.good()) {

		// Read the number of registered collections
		short collections_count = 0;
		collectionsFile.read((char*)&collections_count, sizeof(short));
		if(collectionsFile) {

			collectionsFile.exceptions(
				std::ifstream::failbit |
				std::ifstream::badbit |
				std::ifstream::eofbit
			);

			char* block_buffer = nullptr;
			Collection* collection = nullptr;

			try {
				while(collections_count > 0) {

					// Read block size
					size_t collection_size = 0;
					collectionsFile.read((char*)&collection_size, sizeof(size_t));

					// Read blocks
					block_buffer = new char[collection_size];
					collectionsFile.read(block_buffer, collection_size);

					// Construct the collection
					if(Collection::buildFromBuffer(&collection, block_buffer, collection_size)) {
						collections_preload.insert(
							std::map<std::string, std::shared_ptr<Collection>>::value_type(
								collection->name(),
								collection
							)
						);
					}

					// Update collections_count
					collections_count--;
				}
			}
			catch(std::ifstream::failure e) {
				if(block_buffer != nullptr) {
					delete [] block_buffer;
				}
				if(collection != nullptr) {
					delete collection;
				}
			}
		}
	}

	collectionsFile.close();

	extractFromOBSCollections(collections_preload);
}

void
OBSManager::extractFromOBSCollections(std::map<std::string, std::shared_ptr<Collection>>& collections) {
	bool collections_preload = collections.size() > 0;

	char** obs_collections = obs_frontend_get_scene_collections();

	int i = 0;

	while(obs_collections[i] != NULL) {
		if(collections_preload) {
			auto collection_it = collections.find(obs_collections[i]);
			if(collection_it != collections.end()) {
				m_collections.insert(
					std::map<unsigned long long, std::shared_ptr<Collection>>::value_type(
						collection_it->second->id(),
						collection_it->second
					)
				);
				++i;
				continue;
			}
		}

		++_last_registered_id;
		m_collections.insert(
			std::map<unsigned long long, std::shared_ptr<Collection>>::value_type(
				_last_registered_id,
				new Collection(_last_registered_id, obs_collections[i])
			)
		);
		++i;
	}

	bfree(obs_collections);
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
			++_last_registered_id;
			m_collections.insert(
				std::map<unsigned long long, std::shared_ptr<Collection>>::value_type(
					_last_registered_id,
					new Collection(_last_registered_id, obs_collections[j])
				)
			);
			collection_updated = m_collections.find(_last_registered_id)->second;
			event = obs::collection_event::COLLECTION_ADDED;
		}
		// Update collection
		else if(collections.size() == 1) {
			collections.begin()->second->name(obs_collections[j]);
			collection_updated = collections.begin()->second;
			event = obs::collection_event::COLLECTION_RENAMED;
		}
	}
	else {
		collection_updated = collections.begin()->second;
		m_collections.erase(collections.begin()->second->id());
		event = obs::collection_event::COLLECTION_REMOVED;
	}

	return event;
}

/*Collection*
OBSManager::getCollectionByName(const std::string& name) {
	Collection* collection = nullptr;
	if(m_collections.find(name) != m_collections.end()) {
		collection = &m_collections[name];
	}
	return collection;
}*/

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

/*Collection*
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
}*/