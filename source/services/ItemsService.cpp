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

	this->setupEvent(rpc::event::SHOW_ITEM, &ItemsService::onItemChangeVisibility);
	
	this->setupEvent(rpc::event::HIDE_ITEM, &ItemsService::onItemChangeVisibility);
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

	rpc::response<void> response = response_void(nullptr, "onItemAdded");
	response.event = rpc::event::ITEM_ADDED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
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

	rpc::response<void> response = response_void(nullptr, "onItemRemoved");
	response.event = rpc::event::ITEM_REMOVED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
ItemsService::onItemUpdated(const obs::item::data& data) {

	if(data.item->scene() != data.scene) {
		logError("Something went wrong when deleting item on the current scene.");
		return false;
	}

	switch(data.event) {
		case obs::item::event::HIDDEN:
			data.item->visible(false);
			logInfo(QString("Item %1 hidden.")
				.arg(data.item->name().c_str())
				.toStdString()
			);
			break;
		case obs::item::event::SHOWN:
			data.item->visible(true);
			logInfo(QString("Item %1 shown.")
				.arg(data.item->name().c_str())
				.toStdString()
			);
			break;
	}

	rpc::response<void> response = response_void(nullptr, "onItemUpdated");
	response.event = rpc::event::ITEM_UPDATED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
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

bool
ItemsService::onItemChangeVisibility(const rpc::request& data) {
	rpc::response<rpc::response_error> response = response_error(&data, "onItemChangeVisibility");
	if(data.event == rpc::event::SHOW_ITEM || data.event == rpc::event::HIDE_ITEM) {
		response.event = data.event;
		logInfo("Show/hide item required.");

		if(!checkResource(&data, QRegExp("visibilityItem"))) {
			logWarning("Unknown resource for visibilityItem.");
		}

		if(data.args.size() < 3) {
			response.data.hasMessage = true;
			response.data.error_message = "Not enough argument provided by visibility_item.";
			logError(response.data.error_message);
			streamdeckManager()->commit_to(response, &StreamdeckManager::setError);
			return false;
		}

		QList<QVariant> source_ids = data.args[data.args.count() - 1].toList();
		uint16_t item_id = static_cast<uint16_t>(data.args[data.args.count() - 2].toUInt());
		QList<QVariant> scene_ids = data.args[data.args.count() - 3].toList();

		uint16_t source_collection_id = (uint16_t)source_ids[0].toUInt();
		uint16_t scene_collection_id = (uint16_t)scene_ids[0].toUInt();
		uint16_t current_collection_id = obsManager()->activeCollection()->id();

		uint16_t is_collection = (source_collection_id ^ scene_collection_id ^ current_collection_id);

		if(is_collection != current_collection_id) {
			response.data.hasMessage = true;
			response.data.error_message = "Item, scene and source collection mismatch.";
			logError(response.data.error_message);
			goto send_message;
		}

		uint16_t scene_id = (uint16_t)scene_ids[1].toUInt();
		uint16_t source_id = (uint16_t)source_ids[2].toUInt();
		uint16_t source_flag = (uint16_t)source_ids[1].toUInt();

		Scene* scene = obsManager()->activeCollection()->getSceneById(scene_id);

		if(scene == nullptr) {
			response.data.hasMessage = true;
			response.data.error_message = "The requested scene is not owned by the current collection.";
			logError(response.data.error_message);
			goto send_message;
		}

		Item* item = scene->getItemById(item_id);

		if(item == nullptr) {
			response.data.hasMessage = true;
			response.data.error_message = "The requested item is not owned by the asked scene.";
			logError(response.data.error_message);
			goto send_message;
		}

		if(source_id != item->source()->id()) {
			response.data.hasMessage = true;
			response.data.error_message = "Mismatch between source and item reference.";
			logError(response.data.error_message);
			goto send_message;
		}

		if(!item->visible(data.event == rpc::event::SHOW_ITEM, true)) {
			response.data.hasMessage = true;
			response.data.error_message = "Something went wrong when showing/hiding item.";
			logError(response.data.error_message);
		}
		else
			response.data.error_flag = false;
	}
	else
		logError("visibilityItem not called by VISIBILITY_ITEM");

send_message:
	return streamdeckManager()->commit_to(response, &StreamdeckManager::setError);
}