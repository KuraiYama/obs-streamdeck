/*
 * Plugin Includes
 */
#include "include/obs/Collection.hpp"
#include "include/common/Logger.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Collection::Collection(uint16_t id, std::string name) :
	OBSStorable(id, name),
	m_activeScene(nullptr),
	switching(false),
	active(false),
	m_lastSceneID(0x0),
	m_lastSourceID(0x0) {
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
	short nb_scenes = 0, nb_sources = 0;

	if(memory == nullptr)
		return nullptr;

	memory.read((byte*)&id, sizeof(uint16_t));
	memory.read((byte*)&namelen, sizeof(unsigned int));
	collection_name[namelen] = 0;
	memory.read(collection_name, namelen);

	Collection* collection = new Collection(id, collection_name);

	memory.read((byte*)&nb_sources, sizeof(short));

	while(nb_sources > 0) {
		size_t block_size = 0;
		memory.read((byte*)&block_size, sizeof(size_t));
		void* end_of_block = memory.tell() + block_size;
		Source* source = Source::buildFromMemory(collection, memory);
		if(source != nullptr) {
			collection->m_sources.push(source);
			collection->m_lastSourceID = std::max<uint16_t>(collection->m_lastSourceID, source->id());
		}
		if(memory.tell() != end_of_block) {
			delete collection;
			collection = nullptr;
			nb_sources = 0;
		}
		nb_sources--;
	}

	memory.read((byte*)&nb_scenes, sizeof(short));

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

	std::vector<Memory> blocks;

	/*BLOCK
		id (short)
		namelen (unsigned int)
		name (namelen)
		nbsources (short)
		foreach(source)
			block_size(size_t)
			block_source (block_size)
		nbscenes (short)
		foreach(scene)
			block_size (size_t)
			block_scene (block_size)
	*/
	unsigned int namelen = static_cast<unsigned int>(strlen(m_name.c_str()));
	size_t total_size = sizeof(uint16_t) + sizeof(unsigned int) + namelen + 2*sizeof(short);

	short nb_sources = static_cast<short>(m_sources.size());
	for(auto iter = m_sources.begin(); iter != m_sources.end(); iter++) {
		total_size += sizeof(size_t);
		blocks.push_back(iter->second->toMemory(total_size));
	}

	short nb_scenes = static_cast<short>(m_scenes.size());
	for(auto iter = m_scenes.begin(); iter != m_scenes.end(); iter++) {
		total_size += sizeof(size_t);
		blocks.push_back(iter->second->toMemory(total_size));
	}

	Memory block(total_size);
	block.write((byte*)&m_identifier, sizeof(uint16_t));
	block.write((byte*)&namelen, sizeof(unsigned int));
	block.write((byte*)m_name.c_str(), namelen);

	block.write((byte*)&nb_sources, sizeof(short));
	auto iter = blocks.begin();
	unsigned int i = 0;
	while( i < nb_sources ) {
		size_t sc_size = iter->size();
		block.write((byte*)&sc_size, sizeof(size_t));
		block.write(*iter, iter->size());
		i++;
		iter++;
	}

	block.write((byte*)&nb_scenes, sizeof(short));
	i = 0;
	while ( i < nb_scenes ) {
		size_t sc_size = iter->size();
		block.write((byte*)&sc_size, sizeof(size_t));
		block.write(*iter, iter->size());
		i++;
		iter++;
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

void
Collection::synchronize() {

	auto p = [](void* sources, obs_source_t* obs_source) -> bool {
		OBSStorage<Source> sources_storage = *reinterpret_cast<OBSStorage<Source>*>(sources);
		const char* source_name = obs_source_get_name(obs_source);
		auto source = sources_storage[source_name];
		log_info << QString("Source %1 sourced.").arg(source_name).toStdString() << log_end;
		if(source == nullptr) {
			log_warn << QString("Source %1 doesn't exist.").arg(source_name).toStdString() << log_end;
		}
		else
			source->source(obs_source);
		return true;
	};

	obs_enum_sources(p, &m_sources);

	obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);

	for(size_t i = 0; i < scenes.sources.num; i++) {
		const char* scene = obs_source_get_name(scenes.sources.array[i]);
		Scene* scene_ptr = m_scenes[scene];
		scene_ptr->source(scenes.sources.array[i]);
	}

	obs_frontend_source_list_free(&scenes);
}

/*
========================================================================================================
	Scenes Helpers
========================================================================================================
*/

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
		auto scene_find = scenes.find(iter->second->name());
		if(scene_find == scenes.end()) {
			auto remove = iter;
			iter++;
			m_scenes.pop(remove->first);
		}
		else {
			scenes.erase(scene_find);
			iter++;
		}
	}
	for(auto iter = scenes.begin(); iter != scenes.end(); iter++) {
		m_lastSceneID++;
		m_scenes.push(new Scene(this, m_lastSceneID, *iter));
	}

	bfree(obs_scenes);
}

obs::scene::event
Collection::updateScenes(std::shared_ptr<Scene>& scene_updated) {
	obs::scene::event event = obs::scene::event::LIST_BUILD;
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
		// Fake event - it happens when source and scene triggers both renamed
		if(scenes.size() == 0)
			return event;
		scene_updated = m_scenes.pop(*scenes.begin());
		event = obs::scene::event::REMOVED;
	}
	else {
		const char* name = obs_source_get_name(obs_scenes.sources.array[j]);
		if(scenes.size() == 0) {
			m_lastSceneID++;
			scene_updated = std::shared_ptr<Scene>(new Scene(this, m_lastSceneID, name));
			m_scenes.push(scene_updated);
			scene_updated->source(obs_scenes.sources.array[j]);
			this->makeActive();
			event = obs::scene::event::ADDED;
		}
		else {
			scene_updated = m_scenes.move(*scenes.begin(), name);
			event = obs::scene::event::RENAMED;
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

Scene*
Collection::addScene(obs_source_t* scene) {
	m_lastSceneID++;
	Scene* scene_ref = m_scenes.push(new Scene(this, m_lastSceneID, scene)).get();
	this->makeActive();

	return scene_ref;
}

std::shared_ptr<Scene>
Collection::removeScene(Scene& scene) {
	return m_scenes.pop(scene.id());
}

std::shared_ptr<Scene>
Collection::renameScene(Scene& scene, const char* name) {
	return m_scenes.move(scene.name(), name);
}

Scene*
Collection::getSceneById(uint16_t id) const {
	return m_scenes[id];
}

Scene*
Collection::getSceneByName(const std::string& name) const {
	return m_scenes[name];
}

/*
========================================================================================================
	Sources Helpers
========================================================================================================
*/

void
Collection::loadSources() {
	std::map<std::string, obs_source_t*> sources;

	auto p = [](void* private_data, obs_source_t* obs_source) -> bool {
		auto sources = reinterpret_cast<std::map<std::string, obs_source_t*>*>(private_data);
		const char* name = obs_source_get_name(obs_source);
		sources->insert(std::make_pair(name, obs_source));
		return true;
	};

	obs_enum_sources(p, &sources);

	auto iter = m_sources.begin();
	while(iter != m_sources.end()) {
		auto source_find = sources.find(iter->second->name());
		if(source_find == sources.end()) {
			auto remove = iter;
			iter++;
			m_sources.pop(remove->first);
		}
		else {
			iter->second->source(source_find->second);
			sources.erase(source_find);
			iter++;
		}
	}
	for(auto iter = sources.begin(); iter != sources.end(); iter++) {
		m_lastSourceID++;
		m_sources.push(new Source(this, m_lastSourceID, iter->second));
	}
}

Source*
Collection::addSource(obs_source_t* source) {
	const char* source_name = obs_source_get_name(source);
	if(source_name == NULL) return nullptr;
	Source* existing_source = m_sources[source_name];
	if(existing_source == nullptr) {
		m_lastSourceID++;
		existing_source = m_sources.push(new Source(this, m_lastSourceID, source)).get();
	}
	return existing_source;
}

std::shared_ptr<Source>
Collection::addSource(Source* source) {
	Source* existing_source = m_sources[source->id()];
	if(existing_source != nullptr) {
		m_sources.pop(source->id());
	}
	return m_sources.push(source);
}

std::shared_ptr<Source>
Collection::removeSource(Source& source) {
	return m_sources.pop(source.id());
}

std::shared_ptr<Source>
Collection::removeSource(uint16_t id) {
	return m_sources.pop(id);
}


std::shared_ptr<Source>
Collection::renameSource(Source& source, const char* name) {
	return m_sources.move(source.name(), name);
}

Source*
Collection::getSourceById(uint16_t id) const {
	return m_sources[id];
}

Source*
Collection::getSourceByName(const std::string& name) const {
	return m_sources[name];
}

/*
========================================================================================================
	Accessors
========================================================================================================
*/

Sources
Collection::sources() const {
	Sources sources;
	sources.collection = this;
	for(auto iter = m_sources.begin(); iter != m_sources.end(); iter++)
		if(iter->second->collection() == this)
			sources.sources.push_back(const_cast<Source*>(iter->second.get()));
	for(auto iter = m_scenes.begin(); iter != m_scenes.end(); iter++) {
		if(iter->second->collection() == this)
			sources.sources.push_back(const_cast<Source*>(&iter->second->sourcedScene()));
	}
	return sources;
}

Scenes
Collection::scenes() const {
	Scenes scenes;
	scenes.collection = this;
	for(auto iter = m_scenes.begin(); iter != m_scenes.end(); iter++)
		if(iter->second->collection() == this)
			scenes.scenes.push_back(const_cast<Scene*>(iter->second.get()));
	return scenes;
}

Scene*
Collection::activeScene() const {
	return m_activeScene;
}