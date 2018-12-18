/*
 * Plugin Includes
 */
#include "include/obs/Collection.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Collection::Collection(unsigned long long id, std::string name) :
	m_identifier(id),
	m_name(name) {
}

Collection::~Collection() {
}

/*
========================================================================================================
	Builders
========================================================================================================
*/

bool Collection::buildFromBuffer(Collection** collection, char* buffer, size_t size) {

	char* start_buf = buffer;
	char* end_buf = buffer + size;

	// Extract id
	long long identifier = -1;
	memcpy(&identifier, buffer, sizeof(long long));

	// Extract name length
	buffer += sizeof(long long);
	if(buffer == end_buf) return false;
	size_t nameSz = 0;
	memcpy(&nameSz, buffer, sizeof(size_t));

	// Extract name
	buffer += sizeof(size_t);
	if(buffer == end_buf) return false;
	char* name = new char[nameSz];
	memcpy(name, buffer, nameSz);

	*collection = new Collection(identifier, name);

	// Extract scenes count
	buffer += nameSz;
	if(buffer == end_buf) return false;
	size_t nb_scenes = 0;
	memcpy(&nb_scenes, buffer, sizeof(size_t));

	// Clean memory
	delete [] name;
	delete [] start_buf;

	return buffer == end_buf;
}

/*
========================================================================================================
	Serialization
========================================================================================================
*/

size_t
Collection::toBytes(char** buffer) const {
	if(buffer == nullptr)
		return 0;

	size_t name_len = m_name.length();
	size_t total_size = sizeof(long long) + sizeof(size_t) + name_len;
	*buffer = new char[total_size];
	memcpy(*buffer, &m_identifier, sizeof(long long));
	memcpy(*buffer+sizeof(long long), &name_len, sizeof(size_t));
	memcpy(*buffer+sizeof(long long)+sizeof(size_t), m_name.c_str(), name_len);

	return total_size;
}

/*
========================================================================================================
	Scenes Helpers
========================================================================================================
*/

void
Collection::extractFromOBSScenes(unsigned long long& next_scene_identifier) {
	std::map<std::string, Scene*> scenes;
	for(auto scene_it = m_scenes.begin(); scene_it != m_scenes.end(); scene_it++) {
		scenes.insert(std::map<std::string, Scene*>::value_type(
			scene_it->second->name(),
			scene_it->second.get()
		));
	}

	obs_frontend_set_current_scene_collection(m_name.c_str());
	obs_frontend_source_list obs_scenes = {};
	obs_frontend_get_scenes(&obs_scenes);
	for(size_t i = 0; i < obs_scenes.sources.num; i++) {
		obs_source_t* obs_source = obs_scenes.sources.array[i];
		std::string obs_source_name = obs_source_get_name(obs_source);
		Scene* scene = nullptr;
		if(scenes.size() > 0) {
			auto scene_it = scenes.find(obs_source_name);
			if(scene_it != scenes.end()) {
				scene = scene_it->second;
				scene->source(obs_source);
				scenes.erase(scene_it);
			}
		}
		if(scene == nullptr) {
			m_scenes.insert(std::map<unsigned long long, std::shared_ptr<Scene>>::value_type(
				++next_scene_identifier,
				new Scene(this, next_scene_identifier, obs_source)
			));
		}
	}
	obs_frontend_source_list_free(&obs_scenes);

	if(scenes.size() > 0) {
		for(auto scene_it = scenes.begin(); scene_it != scenes.end(); scene_it++) {
			m_scenes.erase(scene_it->second->id());
		}
	}
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

void
Collection::name(std::string new_name) {
	m_name = new_name;
}

unsigned long long
Collection::id() const {
	return m_identifier;
}

Scenes
Collection::scenes() const {
	Scenes scenes;
	for(auto iter = m_scenes.begin(); iter != m_scenes.end(); iter++)
		scenes.push_back(const_cast<Scene*>(iter->second.get()));
	return scenes;
}