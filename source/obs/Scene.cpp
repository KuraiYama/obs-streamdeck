/*
 * Plugin Includes
 */
#include "include/obs/Scene.hpp"
#include "include/obs/Collection.hpp"
#include "include/common/Logger.hpp"
#include "include/obs/ItemBuilder.hpp"

/*
 * Qt Includes
 */
#include <QString>

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Scene::Scene(Collection* collection, uint16_t id, obs_source_t* source) :
	OBSStorable(id, obs_source_get_name(source)),
	m_parentCollection(collection),
	m_internalSource(collection, id, source, false) {
	this->source(source);
}

Scene::Scene(Collection* collection, uint16_t id, std::string name) :
	OBSStorable(id, name),
	m_parentCollection(collection),
	m_internalSource(collection, id, name, false) {
	m_source = nullptr;
	m_scene = nullptr;
}

Scene::~Scene() {
}

/*
========================================================================================================
	Builders
========================================================================================================
*/

Scene*
Scene::buildFromMemory(Collection* collection, Memory& memory) {
	uint16_t id = 0;
	unsigned int namelen = 0;
	char scene_name[MAX_NAME_LENGTH];

	if(memory == nullptr)
		return nullptr;

	memory.read((byte*)&id, sizeof(uint16_t));
	memory.read((byte*)&namelen, sizeof(unsigned int));
	scene_name[namelen] = 0;
	memory.read(scene_name, namelen);

	Scene* scene = new Scene(collection, id, scene_name);

	return scene;
}

/*
========================================================================================================
	Serialization
========================================================================================================
*/

Memory
Scene::toMemory(size_t& size) const {
	/*BLOCK
		id (short)
		namelen (unsigned int)
		name (namelen)
	*/
	unsigned int namelen = static_cast<unsigned int>(strlen(m_name.c_str()));
	size_t block_size = sizeof(uint16_t) + sizeof(unsigned int) + namelen;

	// TODO

	Memory block(block_size);
	block.write((byte*)&m_identifier, sizeof(uint16_t));
	block.write((byte*)&namelen, sizeof(unsigned int));
	block.write((byte*)m_name.c_str(), namelen);

	size += block_size;
	return block;
}

/*
========================================================================================================
	OBS Helpers
========================================================================================================
*/

bool
Scene::makeActive() {
	obs_frontend_set_current_scene(m_source);
	return true;
}

/*
========================================================================================================
	Item Handling
========================================================================================================
*/

Item*
Scene::createItem(obs_sceneitem_t* item) {
	Item* item_ptr = m_items.push(ItemBuilder::instance()->build(this, item)).get();
	return item_ptr;
}

std::shared_ptr<Item>
Scene::deleteItem(Item* item) {
	std::shared_ptr<Item> item_ptr = m_items.pop(item->id());
	return item_ptr;
}

void
Scene::synchronize() {
	typedef bool (*callback_type)(obs_scene_t* scene, obs_sceneitem_t* item, void* private_data);
	auto func = [](obs_scene_t* scene, obs_sceneitem_t* item, void* private_data) -> bool {
		Scene& scene_ref = *reinterpret_cast<Scene*>(private_data);
		if(scene != scene_ref.scene()) return false;
		uint16_t id = static_cast<uint16_t>(obs_sceneitem_get_id(item));
		auto iter = scene_ref.m_items[id];
		if(iter != nullptr) {
			iter->item(item);
		}
		else {
			scene_ref.m_items.push(ItemBuilder::instance()->build(&scene_ref, item));
		}
		return true;
	};
	obs_scene_enum_items(m_scene, func, this);
}

/*
========================================================================================================
	Accessors
========================================================================================================
*/

Collection*
Scene::collection() const {
	return m_parentCollection;
}

obs_scene_t*
Scene::scene() const {
	return m_scene;
}

obs_source_t*
Scene::source() const {
	return m_source;
}

void
Scene::source(obs_source_t* obs_source) {
	m_source = obs_source;
	m_name = obs_source_get_name(m_source);
	m_scene = obs_scene_from_source(m_source);
	m_internalSource.source(obs_source);
	this->synchronize();
}

Items
Scene::items() const {
	Items items;
	items.scene = this;
	for(auto iter = m_items.begin(); iter != m_items.end(); iter++)
		if(iter->second->scene() == this)
			items.items.push_back(const_cast<Item*>(iter->second.get()));
	return items;
}

unsigned int
Scene::itemCount() const {
	return m_items.size();
}

Source&
Scene::sourcedScene() {
	return m_internalSource;
}