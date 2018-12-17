/*
 * Plugin Includes
 */
#include "include/obs/Collection.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Collection::Collection(long long id, std::string name) :
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

/*void
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
}*/

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

long long
Collection::id() const {
	return m_identifier;
}

/*Scenes
Collection::scenes() const {
	Scenes scenes;
	for(auto iter = m_scenes.begin(); iter != m_scenes.end(); iter++)
		scenes.push_back(const_cast<Scene*>(&(iter->second)));
	return scenes;
}*/