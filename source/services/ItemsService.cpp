/*
 * Plugin Includes
 */
#include "include/services/ItemsService.hpp"
#include "include/common/Logger.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

ItemsService::ItemsService() :
	ServiceImpl("ItemsService", "ScenesService") {

	this->setupEvent(rpc::event::ITEM_ADDED_SUBSCRIBE, &ItemsService::subscribeItemChange);

	this->setupEvent(rpc::event::ITEM_REMOVED_SUBSCRIBE, &ItemsService::subscribeItemChange);

	this->setupEvent(rpc::event::ITEM_UPDATED_SUBSCRIBE, &ItemsService::subscribeItemChange);

	this->setupEvent(obs::item::event::ADDED, &ItemsService::onItemAdded);

	this->setupEvent(obs::item::event::REMOVED, &ItemsService::onItemRemoved);

	this->setupEvent(obs::item::event::HIDDEN, &ItemsService::onItemUpdated);

	this->setupEvent(obs::item::event::SHOWN, &ItemsService::onItemUpdated);

}

ItemsService::~ItemsService() {
}

/*
========================================================================================================
	OBS Event Handling
========================================================================================================
*/

bool
ItemsService::onItemAdded(const obs::item::data& data) {

	Item* item = data.scene->createItem(data.sceneitem);

	if(item == nullptr) {
		logError("Something went wrong when creating item on the current scene.");
		return false;
	}

	logInfo(QString("Item %1 added on scene %2.")
		.arg(item->name().c_str())
		.arg(item->scene()->name().c_str())
		.toStdString()
	);

	obsManager()->registerItem(item);

	/*rpc::response<void> response = response_void(nullptr, "onSceneAdded");
	response.event = rpc::event::SCENE_ADDED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);*/
	return true;
}

bool
ItemsService::onItemRemoved(const obs::item::data& data) {

	std::shared_ptr<Item> item_ptr = data.scene->deleteItem(data.item);

	if(item_ptr == nullptr) {
		logError("Something went wrong when deleting item on the current scene.");
		return false;
	}

	logInfo(QString("Item %1 removed from scene %2.")
		.arg(item_ptr->name().c_str())
		.arg(item_ptr->scene()->name().c_str())
		.toStdString()
	);

	obsManager()->unregisterItem(item_ptr.get());

	/*rpc::response<void> response = response_void(nullptr, "onSceneRemoved");
	response.event = rpc::event::SCENE_REMOVED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);*/
	return true;
}

bool
ItemsService::onItemUpdated(const obs::item::data& data) {

	if(data.item->scene() != data.scene) {
		logError("Something went wrong when deleting item on the current scene.");
		return false;
	}

	switch(data.event) {
		case obs::item::event::HIDDEN:
			logInfo(QString("Item %1 hidden.")
				.arg(data.item->name().c_str())
				.toStdString()
			);
			break;
		case obs::item::event::SHOWN:
			logInfo(QString("Item %1 shown.")
				.arg(data.item->name().c_str())
				.toStdString()
			);
			break;
	}

	/*logInfo(QString("Scene renamed to %1").arg(scene.name().c_str()).toStdString());

	// The RPC protocol doesn't provide any resource for handling scene renaming.
	// We can use both scene removed/scene added to handle that, but each of them
	// implies GET_SCENES message. Then we send directly the GET_SCENES message instead.

	rpc::response<Scenes> response = response_scenes(nullptr, "onSceneUpdated");
	response.event = rpc::event::GET_SCENES;
	Collection* collection = scene.collection();
	response.data = collection->scenes();

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setScenes);*/
	return true;
}

/*
========================================================================================================
	RPC Event Handling
========================================================================================================
*/

bool
ItemsService::subscribeItemChange(const rpc::request& data) {
	rpc::response<std::string> response = response_string(&data, "subscribeItemChange");
	if(data.event == rpc::event::ITEM_ADDED_SUBSCRIBE ||
		data.event == rpc::event::ITEM_REMOVED_SUBSCRIBE ||
		data.event == rpc::event::ITEM_UPDATED_SUBSCRIBE
		) {
		response.event = data.event;
		logInfo("Subscription to item event required");

		if(!checkResource(&data, QRegExp("(.+)"))) {
			// This streamdeck doesn't provide any resource to warn on stream state change
			logError("Streamdeck didn't provide resourceId to subscribe.");
			return false;
		}

		response.data = QString("%1.%2")
			.arg(data.serviceName.c_str())
			.arg(data.method.c_str())
			.toStdString();

		return streamdeckManager()->commit_to(response, &StreamdeckManager::setSubscription);
	}

	logError("subscribeItemChange not called by ITEM_SUBSCRIBE");
	return false;
}