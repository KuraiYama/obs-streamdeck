/*
 * Plugin Includes
 */
#include "include/services/CollectionsService.hpp"
#include "include/common/Logger.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

CollectionsService::CollectionsService() : ServiceT("CollectionsService", "SceneCollectionsService") {

	this->setupEvent(obs_frontend_event::OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED,
		&CollectionsService::onCollectionsListChanged);

	this->setupEvent(obs_frontend_event::OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED,
		&CollectionsService::onCollectionSwitched);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::FETCH_COLLECTIONS_SCHEMA,
		&CollectionsService::onFetchCollectionsSchema);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::GET_COLLECTIONS,
		&CollectionsService::onGetCollections);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::GET_ACTIVE_COLLECTION,
		&CollectionsService::onGetActiveCollection);

	/*this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::RPC_ID_MAKE_COLLECTION_ACTIVE,
		&CollectionsService::onMakeCollectionActive);*/

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::COLLECTION_ADDED_SUBSCRIBE,
		&CollectionsService::subscribeCollectionChange);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::COLLECTION_REMOVED_SUBSCRIBE,
		&CollectionsService::subscribeCollectionChange);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::COLLECTION_UPDATED_SUBSCRIBE,
		&CollectionsService::subscribeCollectionChange);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::COLLECTION_SWITCHED_SUBSCRIBE,
		&CollectionsService::subscribeCollectionChange);
}

CollectionsService::~CollectionsService() {
}

/*
========================================================================================================
	OBS Event Handling
========================================================================================================
*/

bool
CollectionsService::onCollectionsListChanged() {
	std::shared_ptr<Collection> collection_updated = nullptr;
	obs::collection_event evt = obsManager()->updateCollections(collection_updated);
	switch(evt) {
		case obs::collection_event::COLLECTION_ADDED:
			return onCollectionAdded(*collection_updated.get());
			break;
		case obs::collection_event::COLLECTION_REMOVED:
			return onCollectionRemoved(*collection_updated.get());
			break;
		case obs::collection_event::COLLECTION_RENAMED:
			return onCollectionUpdated(*collection_updated.get());
			break;
	}
	return false;
}

bool
CollectionsService::onCollectionSwitched() {

	// During loading, we don't send anything to the streamdecks
	if(obsManager()->isLoadingCollections())
		return true;

	Collection* collection = obsManager()->activeCollection();
	logInfo(QString("Collection switched to %1.")
		.arg(collection->name().c_str())
		.toStdString()
	);

	rpc_adv_response<CollectionPtr> response = response_collection(nullptr, "onCollectionSwitched");
	response.event = Streamdeck::rpc_event::COLLECTION_SWITCHED_SUBSCRIBE;
	response.data = collection;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
CollectionsService::onCollectionAdded(const Collection& collection) {
	logInfo(QString("Collection %1 (%2) added.")
		.arg(collection.name().c_str())
		.arg(collection.id())
		.toStdString()
	);

	rpc_adv_response<void> response = response_void(nullptr, "onCollectionAdded");
	response.event = Streamdeck::rpc_event::COLLECTION_ADDED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
CollectionsService::onCollectionRemoved(const Collection& collection) {
	logInfo(QString("Collection %1 (%2) removed.")
		.arg(collection.name().c_str())
		.arg(collection.id())
		.toStdString()
	);

	rpc_adv_response<void> response = response_void(nullptr, "onCollectionRemoved");
	response.event = Streamdeck::rpc_event::COLLECTION_REMOVED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
CollectionsService::onCollectionUpdated(const Collection& collection) {
	logInfo(QString("Collection %1 (%2) renamed.")
		.arg(collection.name().c_str())
		.arg(collection.id())
		.toStdString()
	);

	rpc_adv_response<CollectionPtr> response = response_collection(nullptr, "onCollectionUpdated");
	response.event = Streamdeck::rpc_event::COLLECTION_UPDATED_SUBSCRIBE;
	response.data = const_cast<CollectionPtr>(&collection);

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

/*
========================================================================================================
	RPC Event Handling
========================================================================================================
*/

bool
CollectionsService::subscribeCollectionChange(const rpc_event_data& data) {
	rpc_adv_response<std::string> response = response_string(&data, "subscribeCollectionChange");
	if(data.event == Streamdeck::rpc_event::COLLECTION_ADDED_SUBSCRIBE ||
		data.event == Streamdeck::rpc_event::COLLECTION_REMOVED_SUBSCRIBE || 
		data.event == Streamdeck::rpc_event::COLLECTION_UPDATED_SUBSCRIBE ||
		data.event == Streamdeck::rpc_event::COLLECTION_SWITCHED_SUBSCRIBE
	) {
		response.event = data.event;
		logInfo("Subscription to collection event required");

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

	logError("subscribeCollectionChange not called by COLLECTION_SUBSCRIBE");
	return false;
}

bool
CollectionsService::onFetchCollectionsSchema(const rpc_event_data& data) {
	rpc_adv_response<std::string> response = response_string(&data, "onFetchCollectionsSchema");
	if(data.event == Streamdeck::rpc_event::FETCH_COLLECTIONS_SCHEMA) {
		response.event = Streamdeck::rpc_event::FETCH_COLLECTIONS_SCHEMA;
		logInfo("Fetching schemas required...");

		if(!checkResource(&data, QRegExp("(.+)"))) {
			// This streamdeck doesn't provide any resource to warn on stream state change
			logError("Streamdeck didn't provide resourceId to subscribe.");
			return false;
		}

		response.data = QString("%1.%2")
			.arg(data.serviceName.c_str())
			.arg(data.method.c_str())
			.toStdString();
	}

	if(!streamdeckManager()->commit_to(response, &StreamdeckManager::setSubscription))
		return false;

	rpc_adv_response<Collections> response2 = response_collections(&data, "onFetchCollectionsSchema");
	response2.event = Streamdeck::rpc_event::FETCH_COLLECTIONS_SCHEMA;
	response2.data = obsManager()->collections();

	return streamdeckManager()->commit_to(response2, &StreamdeckManager::setSchema);
}

bool
CollectionsService::onGetCollections(const rpc_event_data& data) {
	
	rpc_adv_response<Collections> response = response_collections(&data, "onGetCollections");
	if(data.event == Streamdeck::rpc_event::GET_COLLECTIONS) {
		response.event = Streamdeck::rpc_event::GET_COLLECTIONS;
		logInfo("Collections list required.");

		if(!checkResource(&data, QRegExp("getCollections"))) {
			logWarning("Unknown resource for getCollections.");
		}

		response.data = obsManager()->collections();
		return streamdeckManager()->commit_to(response, &StreamdeckManager::setCollections);
	}

	logError("GetCollections not called by GET_COLLECTIONS.");
	return false;
}

bool
CollectionsService::onGetActiveCollection(const rpc_event_data& data) {
	rpc_adv_response<CollectionPtr> response = response_collection(&data, "onGetActiveCollection");
	if(data.event == Streamdeck::rpc_event::GET_ACTIVE_COLLECTION) {
		response.event = Streamdeck::rpc_event::GET_ACTIVE_COLLECTION;
		if(!checkResource(&data, QRegExp("activeCollection"))) {
			logWarning("Unknown resource for activeCollection.");
		}

		response.data = obsManager()->activeCollection();
		return streamdeckManager()->commit_to(response, &StreamdeckManager::setCollection);
	}

	logError("GetActiveCollection not called by GET_ACTIVE_COLLECTION.");
	return false;
}

/*bool
CollectionsService::onMakeCollectionActive(const rpc_event_data& data) {
	rpc_adv_response<bool> response = response_bool(&data, "onMakeCollectionActive");
	if(data.event == Streamdeck::rpc_event::RPC_ID_MAKE_COLLECTION_ACTIVE) {
		response.event = Streamdeck::rpc_event::RPC_ID_MAKE_COLLECTION_ACTIVE;
		if(data.serviceName.compare("SceneCollectionsService") == 0
				&& data.method.compare("activeCollection") == 0) {

			// TODO

			response.data = false;
			return streamdeckManager()->commit_to(response, &StreamdeckManager::setError);
		}
	}

	return streamdeckManager()->commit_to(response, &StreamdeckManager::setError);
}*/