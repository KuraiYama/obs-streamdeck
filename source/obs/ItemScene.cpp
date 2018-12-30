/*
 * Plugin Includes
 */
#include "include/obs/Scene.hpp"
#include "include/obs/ItemScene.hpp"

/*
 * Qt Includes
 */
#include <QString>

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

ItemScene::ItemScene(Scene* scene, uint16_t id, obs_sceneitem_t* item) :
	Item(scene, id, item) {
}

ItemScene::ItemScene(Scene* scene, uint16_t id, const std::string& name) :
	Item(scene, id, name) {
}

ItemScene::~ItemScene() {
}

/*
========================================================================================================
	Accessors
========================================================================================================
*/

const char*
ItemScene::type() const {
	return "item";
}