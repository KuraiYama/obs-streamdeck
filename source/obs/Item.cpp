/*
 * Plugin Includes
 */
#include "include/obs/Item.hpp"
#include "include/obs/Scene.hpp"

/*
 * Qt Includes
 */
#include <QString>

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Item::Item(Scene* scene, obs_sceneitem_t* item) : m_parentScene(scene), m_sceneItem(item) {
	m_source = obs_sceneitem_get_source(m_sceneItem);
	m_name = obs_source_get_name(m_source);
}

Item::~Item() {
}

/*
========================================================================================================
	Accessors
========================================================================================================
*/

std::string Item::name() const {
	return m_name;
}

std::string Item::completeName() const {
	return QString("%1.%2")
		.arg(m_parentScene->id().c_str())
		.arg(m_name.c_str())
		.toStdString();
}
const char* Item::type() const {
	return "item";
}

int64_t Item::id() const {
	return 54;// obs_sceneitem_get_id(m_sceneItem);
}

bool Item::visible() const {
	return true;// obs_sceneitem_visible(m_sceneItem);
}