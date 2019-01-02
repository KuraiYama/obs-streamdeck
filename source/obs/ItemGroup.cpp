/*
 * Plugin Includes
 */
#include "include/obs/Scene.hpp"
#include "include/obs/ItemGroup.hpp"

/*
 * Qt Includes
 */
#include <QString>

/*
========================================================================================================
	Static Variables Initialization
========================================================================================================
*/

bool ItemGroup::_toggle_subitems = false;

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

ItemGroup::ItemGroup(Scene* scene, uint16_t id, obs_sceneitem_t* item) :
	Item(scene, id, item) {
	typedef bool(*callback_type)(obs_scene_t* scene, obs_sceneitem_t* item, void* private_data);
	auto func = [](obs_scene_t* scene, obs_sceneitem_t* item, void* private_data) -> bool {
		ItemGroup* owner = reinterpret_cast<ItemGroup*>(private_data);
		Scene* scene_ref = owner->scene();
		uint16_t id = static_cast<uint16_t>(obs_sceneitem_get_id(item));
		auto iter = scene_ref->getItemById(id);
		if(iter != nullptr) {
			iter->item(item);
			owner->add(iter);
		}
		else {
			owner->add(scene_ref->createItem(item));
		}
		return true;
	};
	obs_scene_enum_items(obs_group_from_source(m_source), func, this);

	m_toggling = false;
}

ItemGroup::ItemGroup(Scene* scene, uint16_t id, const std::string& name) :
	Item(scene, id, name) {
	m_toggling = false;
}

ItemGroup::~ItemGroup() {
}

/*
========================================================================================================
	Items Handling
========================================================================================================
*/

void
ItemGroup::remove(Item* item) {
	m_items.erase(item);
}

void
ItemGroup::add(Item* item) {
	item->m_ownerItem = this;
	if(m_items.find(item) == m_items.end())
		m_items.emplace(item);
}

void
ItemGroup::item(obs_sceneitem_t* item) {
	Item::item(item);
	typedef bool(*callback_type)(obs_scene_t* scene, obs_sceneitem_t* item, void* private_data);
	auto func = [](obs_scene_t* scene, obs_sceneitem_t* item, void* private_data) -> bool {
		ItemGroup* group = reinterpret_cast<ItemGroup*>(private_data);
		Scene* scene_ref = group->scene();
		uint16_t id = static_cast<uint16_t>(obs_sceneitem_get_id(item));
		auto iter = scene_ref->getItemById(id);
		if(iter != nullptr) {
			iter->item(item);
			group->add(iter);
		}
		return true;
	};
	obs_scene_enum_items(obs_group_from_source(m_source), func, this);
}

bool
ItemGroup::visible() const {
	return Item::visible();
}

bool
ItemGroup::visible(bool toggle, bool rpc_action) {
	if(m_toggling) return true;

	if(_toggle_subitems == false || !rpc_action)
		return Item::visible(toggle, rpc_action);
	else {
		m_toggling = true;
		bool result = true;
		for(auto iter = m_items.begin(); iter != m_items.end(); iter++)
			result &= (*iter)->visible(toggle, true);
		m_visible = toggle;
		obs_sceneitem_set_visible(m_item, false);
		obs_sceneitem_set_visible(m_item, true);

		m_toggling = false;
		return result;
	}
}

/*
========================================================================================================
	Accessors
========================================================================================================
*/

const char*
ItemGroup::type() const {
	return "item";
}