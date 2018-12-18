/*
 * Plugin Includes
 */
#include "include/obs/Scene.hpp"
#include "include/obs/Collection.hpp"

/*
 * Qt Includes
 */
#include <QString>

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Scene::Scene(Collection* collection, unsigned long long id, obs_source_t* source) :
	m_parentCollection(collection),
	m_source(source) {
	m_name = obs_source_get_name(m_source);
	m_scene = obs_scene_from_source(m_source);
}

Scene::Scene(Collection* collection, unsigned long long id, std::string name) :
	m_parentCollection(collection),
	m_identifier(id),
	m_name(name) {
	m_source = nullptr;
	m_scene = nullptr;
}

Scene::~Scene() {
}

/*
========================================================================================================
	Item Handling
========================================================================================================
*/

/*void
Scene::buildItems() {
	auto add_item_func = [](obs_scene_t* scene, obs_sceneitem_t* item, void* param)->bool {
		Scene& sceneObject = *reinterpret_cast<Scene*>(param);

		if(sceneObject.scene() != scene)
			return false;
		
		Item itemObject(&sceneObject, item);
		sceneObject.m_items.insert(
			std::map<int64_t, Item>::value_type(itemObject.id(), std::move(itemObject)));
		return true;
	};

	obs_scene_enum_items(m_scene, add_item_func, this);
}*/

/*
========================================================================================================
	Accessors
========================================================================================================
*/

std::string
Scene::name() const {
	return m_name;
}

unsigned long long
Scene::id() const {
	return m_identifier;
}

obs_scene_t*
Scene::scene() const {
	return m_scene;
}

void
Scene::source(obs_source_t* obs_source) {
	m_source = obs_source;
	m_scene = obs_scene_from_source(obs_source);
}

/*Items
Scene::items() const {
	Items items;
	for(auto iter = m_items.begin(); iter != m_items.end(); iter++)
		items.push_back(const_cast<Item*>(&(iter->second)));
	return items;
}*/