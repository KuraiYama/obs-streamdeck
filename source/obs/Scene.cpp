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

Scene::Scene(Collection* collection, obs_source_t* source) : 
	m_parentCollection(collection),
	m_source(source) {
	m_name = obs_source_get_name(m_source);
	m_scene = obs_scene_from_source(m_source);
}

Scene::Scene(const Scene& scene) {
	m_parentCollection = scene.m_parentCollection;
	m_source = scene.m_source;
	m_scene = scene.m_scene;
	m_name = scene.m_name;
	m_items = scene.m_items;
}

Scene::Scene(Scene&& scene) {
	m_parentCollection = scene.m_parentCollection;
	m_source = scene.m_source;
	m_scene = scene.m_scene;
	m_name = std::move(scene.m_name);
	m_items = std::move(scene.m_items);
}

Scene::~Scene() {
}

/*
========================================================================================================
	Item Handling
========================================================================================================
*/

void
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
}

/*
========================================================================================================
	Accessors
========================================================================================================
*/

std::string
Scene::name() const {
	return m_name;
}

std::string
Scene::id() const {
	return QString("%1.%2")
		.arg(m_parentCollection->id().c_str())
		.arg(m_name.c_str()).toStdString();
}

obs_scene_t*
Scene::scene() const {
	return m_scene;
}

Items
Scene::items() const {
	Items items;
	for(auto iter = m_items.begin(); iter != m_items.end(); iter++)
		items.push_back(const_cast<Item*>(&(iter->second)));
	return items;
}