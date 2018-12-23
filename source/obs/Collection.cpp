/*
 * Plugin Includes
 */
#include "include/obs/Collection.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Collection::Collection(uint16_t id, std::string name) :
	OBSStorable(id, name),
	m_activeScene(nullptr),
	switching(false),
	m_lastSceneID(0x0) {
}

Collection::~Collection() {
}

/*
========================================================================================================
	Builders
========================================================================================================
*/

Collection*
Collection::buildFromMemory(Memory& memory) {
	uint16_t id = 0;
	unsigned int namelen = 0;
	char collection_name[MAX_NAME_LENGTH];
	short nb_scenes = 0;

	if(memory == nullptr)
		return nullptr;

	memory.read((byte*)&id, sizeof(uint16_t));
	memory.read((byte*)&namelen, sizeof(unsigned int));
	collection_name[namelen] = 0;
	memory.read(collection_name, namelen);
	memory.read((byte*)&nb_scenes, sizeof(short));

	Collection* collection = new Collection(id, collection_name);

	while(nb_scenes > 0) {
		size_t block_size = 0;
		memory.read((byte*)&block_size, sizeof(size_t));
		void* end_of_block = memory.tell() + block_size;
		Scene* scene = Scene::buildFromMemory(collection, memory);
		if(scene != nullptr) {
			collection->m_scenes.push(scene);
			collection->m_lastSceneID = std::max<uint16_t>(collection->m_lastSceneID, scene->id());
		}
		if(memory.tell() != end_of_block) {
			delete collection;
			collection = nullptr;
			nb_scenes = 0;
		}
		nb_scenes--;
	}

	return collection;
}

/*
========================================================================================================
	Serialization
========================================================================================================
*/

Memory
Collection::toMemory(size_t& size) const {

	std::vector<Memory> scene_blocks;

	/*BLOCK
		id (unsigned long long)
		namelen (unsigned int)
		name (namelen)
		nbscenes (short)
		foreach(scene)
			block_size (size_t)
			block_scene (block_size)
	*/
	unsigned int namelen = static_cast<unsigned int>(strlen(m_name.c_str()));
	size_t total_size = sizeof(uint16_t) + sizeof(unsigned int) + namelen + sizeof(short);
	short nb_scenes = static_cast<short>(m_scenes.size());
	for(auto iter = m_scenes.begin(); iter != m_scenes.end(); iter++) {
		total_size += sizeof(size_t);
		scene_blocks.push_back(iter->second->toMemory(total_size));
	}

	Memory block(total_size);
	block.write((byte*)&m_identifier, sizeof(uint16_t));
	block.write((byte*)&namelen, sizeof(unsigned int));
	block.write((byte*)m_name.c_str(), namelen);
	block.write((byte*)&nb_scenes, sizeof(short));

	for(auto iter = scene_blocks.begin(); iter < scene_blocks.end(); iter++) {
		size_t sc_size = iter->size();
		block.write((byte*)&sc_size, sizeof(size_t));
		block.write(*iter, iter->size());
	}

	size += total_size;
	return block;
}

/*
========================================================================================================
	OBS Helpers
========================================================================================================
*/

void
Collection::makeActive() {
	obs_source_t* current_scene = obs_frontend_get_current_scene();
	const char* name = obs_source_get_name(current_scene);
	obs_source_release(current_scene);
	m_activeScene = m_scenes[name];
}

/*
========================================================================================================
	Scenes Helpers
========================================================================================================
*/

void
Collection::synchronize() {
	obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);

	for(size_t i = 0; i < scenes.sources.num; i++) {
		const char* scene = obs_source_get_name(scenes.sources.array[i]);
		Scene* scene_ptr = m_scenes[scene];
		scene_ptr->source(scenes.sources.array[i]);
	}

	obs_frontend_source_list_free(&scenes);
}

void
Collection::loadScenes() {
	std::set<std::string> scenes;
	char** obs_scenes = obs_frontend_get_scene_names();
	unsigned int i = 0;
	while(obs_scenes[i] != NULL) {
		scenes.insert(std::string(obs_scenes[i]));
		++i;
	}
	auto iter = m_scenes.begin();
	while(iter != m_scenes.end()) {
		if(scenes.find(iter->second->name()) == scenes.end()) {
			auto remove = iter;
			iter++;
			m_scenes.pop(remove->first);
		}
		else {
			scenes.erase(iter->second->name());
			iter->second->loadItems();
			iter++;
		}
	}
	for(auto iter = scenes.begin(); iter != scenes.end(); iter++) {
		m_lastSceneID++;
		m_scenes.push(new Scene(this, m_lastSceneID, *iter))->loadItems();
	}
	bfree(obs_scenes);

	// We synchronized the first time
}

obs::scene_event
Collection::updateScenes(std::shared_ptr<Scene>& scene_updated) {
	obs::scene_event event = obs::scene_event::SCENES_LIST_BUILD;
	std::set<std::string> scenes;
	for(auto iter = m_scenes.begin(); iter != m_scenes.end(); iter++) {
		scenes.insert(iter->second->name());
	}

	obs_frontend_source_list obs_scenes = {};
	obs_frontend_get_scenes(&obs_scenes);

	int64_t j = -1;
	for(int64_t i = 0; i < obs_scenes.sources.num; i++) {
		auto iter = scenes.find(obs_source_get_name(obs_scenes.sources.array[i]));
		if(iter != scenes.end()) {
			scenes.erase(iter);
		}
		else {
			j = i;
		}
	}

	if(j == -1) {
		scene_updated = m_scenes.pop(*scenes.begin());
		event = obs::scene_event::SCENE_REMOVED;
	}
	else {
		const char* name = obs_source_get_name(obs_scenes.sources.array[j]);
		if(scenes.size() == 0) {
			m_lastSceneID++;
			scene_updated = std::shared_ptr<Scene>(new Scene(this, m_lastSceneID, name));
			m_scenes.push(scene_updated);
			scene_updated->source(obs_scenes.sources.array[j]);
			this->makeActive();
			event = obs::scene_event::SCENE_ADDED;
		}
		else {
			scene_updated = m_scenes.move(*scenes.begin(), name);
			event = obs::scene_event::SCENE_RENAMED;
		}
	}

	obs_frontend_source_list_free(&obs_scenes);

	return event;
}

bool
Collection::switchScene(uint16_t id) {
	Scene* scene = m_scenes[id];
	if(scene != nullptr)
		obs_frontend_set_current_scene(obs_scene_get_source(scene->scene()));
	return scene != nullptr;
}

bool
Collection::switchScene(const char* name) {
	if(name == NULL) return false;
	Scene* scene = m_scenes[name];
	if(scene != nullptr)
		obs_frontend_set_current_scene(obs_scene_get_source(scene->scene()));
	return scene != nullptr;
}

/*
========================================================================================================
	Accessors
========================================================================================================
*/

Scenes
Collection::scenes() const {
	Scenes scenes;
	scenes._collection = this;
	for(auto iter = m_scenes.begin(); iter != m_scenes.end(); iter++)
		if(iter->second->collection() == this)
			scenes._scenes.push_back(const_cast<Scene*>(iter->second.get()));
	return scenes;
}

Scene*
Collection::activeScene() const {
	return m_activeScene;
}