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

CollectionsService::CollectionsService(StreamdeckManager* streamdeckManager, 
		CollectionManager* collectionManager) : ServiceT("CollectionsService", streamdeckManager) {

	m_collectionManager = collectionManager;

	this->setupEvent(obs_frontend_event::OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED,
		&CollectionsService::onCollectionsListChanged);

	this->setupEvent(obs_frontend_event::OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED,
		&CollectionsService::onCollectionSwitched);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::RPC_ID_FETCH_COLLECTIONS_SCHEMA,
		&CollectionsService::onFetchCollectionsSchema);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::RPC_ID_GET_COLLECTIONS,
		&CollectionsService::onFetchCollectionsSchema);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::RPC_ID_GET_ACTIVE_COLLECTION,
		&CollectionsService::onGetActiveCollection);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::RPC_ID_MAKE_COLLECTION_ACTIVE,
		&CollectionsService::onMakeCollectionActive);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::RPC_ID_COLLECTION_ADDED_SUBSCRIBE,
		&CollectionsService::subscribeCollectionChange);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::RPC_ID_COLLECTION_REMOVED_SUBSCRIBE,
		&CollectionsService::subscribeCollectionChange);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::RPC_ID_COLLECTION_UPDATED_SUBSCRIBE,
		&CollectionsService::subscribeCollectionChange);

	//this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::RPC_ID_COLLECTION_SWITCHED_SUBSCRIBE,
		//&CollectionsService::subscribeCollectionChange);
}

CollectionsService::~CollectionsService() {
}

/*
========================================================================================================
	OBS Event Handling
========================================================================================================
*/

bool CollectionsService::onCollectionsListChanged() {
	CollectionManager::obs_collection_event evt = m_collectionManager->buildCollections();

	switch(evt) {
		case CollectionManager::obs_collection_event::OBS_COLLECTION_EVENT_ADDED:
			return onCollectionAdded();
			break;

		case CollectionManager::obs_collection_event::OBS_COLLECTION_EVENT_REMOVED:
			return onCollectionRemoved();
			break;

		case CollectionManager::obs_collection_event::OBS_COLLECTION_EVENT_UPDATED:
			return onCollectionUpdated();
			break;

		default:
			break;
	}

	return false;
}

bool CollectionsService::onCollectionSwitched() {

	// Switch during collections building is normal
	// We don't notify streamdecks
	if(m_collectionManager->isBuildingCollections()) return true;

	rpc_adv_response<Collection*> response = response_collection(nullptr, "onCollectionSwitched");
	response.event = Streamdeck::rpc_event::RPC_ID_COLLECTION_SWITCHED_SUBSCRIBE;
	response.data = m_collectionManager->activeCollection();
	return streamdeckManager()->commit_all(response, &StreamdeckManager::setCollectionSwitched);
}

bool CollectionsService::onCollectionAdded() {
	rpc_adv_response<void> response = response_void(nullptr, "onCollectionAdded");
	response.event = Streamdeck::rpc_event::RPC_ID_COLLECTION_ADDED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setStatus);
}

bool CollectionsService::onCollectionRemoved() {
	rpc_adv_response<void> response = response_void(nullptr, "onCollectionRemoved");
	response.event = Streamdeck::rpc_event::RPC_ID_COLLECTION_REMOVED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setStatus);
}

bool CollectionsService::onCollectionUpdated() {
	rpc_adv_response<Collections> response = response_collections(nullptr, "onCollectionUpdated");
	response.event = Streamdeck::rpc_event::RPC_ID_COLLECTION_UPDATED_SUBSCRIBE;
	response.data = m_collectionManager->collections();

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setCollections);
}

/*
========================================================================================================
	RPC Event Handling
========================================================================================================
*/

bool CollectionsService::subscribeCollectionChange(const rpc_event_data& data) {
	rpc_adv_response<std::string> response = response_string(&data, "subscribeCollectionChange");
	if(data.event == Streamdeck::rpc_event::RPC_ID_COLLECTION_ADDED_SUBSCRIBE ||
		data.event == Streamdeck::rpc_event::RPC_ID_COLLECTION_REMOVED_SUBSCRIBE || 
		data.event == Streamdeck::rpc_event::RPC_ID_COLLECTION_UPDATED_SUBSCRIBE || 
		data.event == Streamdeck::rpc_event::RPC_ID_COLLECTION_SWITCHED_SUBSCRIBE) {
		response.event = data.event;
		logger("Subscription to collection event required");
		if(data.serviceName.empty() || data.method.empty()) {
			// This streamdeck doesn't provide any resource to warn on stream state change
			return false;
		}
		response.data = QString("%1.%2")
			.arg(data.serviceName.c_str())
			.arg(data.method.c_str())
			.toStdString();
	}

	return streamdeckManager()->commit_to(response, &StreamdeckManager::setSubscription);
}

bool CollectionsService::onFetchCollectionsSchema(const rpc_event_data& data) {
	rpc_adv_response<std::string> response = response_string(&data, "onFetchCollectionsSchema");
	if(data.event == Streamdeck::rpc_event::RPC_ID_FETCH_COLLECTIONS_SCHEMA) {
		response.event = Streamdeck::rpc_event::RPC_ID_FETCH_COLLECTIONS_SCHEMA;
		logger("Streamdeck has required fetch collection schema...");
		if(data.serviceName.empty() || data.method.empty()) {
			// This streamdeck doesn't provide any resource to warn on fetch schema
			return false;
		}
		response.data = QString("%1.%2")
			.arg(data.serviceName.c_str())
			.arg(data.method.c_str())
			.toStdString();
	}

	return streamdeckManager()->commit_to(response, &StreamdeckManager::setSubscription) && 
		this->onGetCollections(data);
}

bool CollectionsService::onGetCollections(const rpc_event_data& data) {
	
	rpc_adv_response<Collections> response =
		response_collections(&data, "onGetCollections");
	
	bool result = false;

	// Fetch case
	response.data = m_collectionManager->collections();
	if(data.event == Streamdeck::rpc_event::RPC_ID_FETCH_COLLECTIONS_SCHEMA) {
		response.event = Streamdeck::rpc_event::RPC_ID_FETCH_COLLECTIONS_SCHEMA;
		result = streamdeckManager()->commit_to(response, &StreamdeckManager::fetchCollections);
	}
	
	else if(data.event == Streamdeck::rpc_event::RPC_ID_GET_COLLECTIONS) {
		response.event = Streamdeck::rpc_event::RPC_ID_GET_COLLECTIONS;
		result = streamdeckManager()->commit_to(response, &StreamdeckManager::setCollections);
	}

	return result;
}

bool CollectionsService::onGetActiveCollection(const rpc_event_data& data) {
	rpc_adv_response<Collection*> response = response_collection(&data, "onGetActiveCollection");
	if(data.event == Streamdeck::rpc_event::RPC_ID_GET_ACTIVE_COLLECTION) {
		response.event = Streamdeck::rpc_event::RPC_ID_GET_ACTIVE_COLLECTION;
		if(data.serviceName.compare("SceneCollectionsService") == 0
				&& data.method.compare("activeCollection") == 0) {
			response.data = m_collectionManager->activeCollection();
			return streamdeckManager()->commit_to(response, &StreamdeckManager::setActiveCollection);
		}
	}

	return false;
}

bool CollectionsService::onMakeCollectionActive(const rpc_event_data& data) {
	rpc_adv_response<bool> response = response_bool(&data, "onMakeCollectionActive");
	if(data.event == Streamdeck::rpc_event::RPC_ID_MAKE_COLLECTION_ACTIVE) {
		response.event = Streamdeck::rpc_event::RPC_ID_MAKE_COLLECTION_ACTIVE;
		if(data.serviceName.compare("SceneCollectionsService") == 0
				&& data.method.compare("activeCollection") == 0) {

			/* TODO */

			response.data = false;
			return streamdeckManager()->commit_to(response, &StreamdeckManager::setError);
		}
	}

	return streamdeckManager()->commit_to(response, &StreamdeckManager::setError);
}