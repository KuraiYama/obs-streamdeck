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

CollectionsService::CollectionsService() :
	ServiceImpl("CollectionsService", "SceneCollectionsService"),
	m_collectionToSwitch(0x0),
	m_collectionUpdated(nullptr) {

	this->setupEvent(obs::save::event::LOADING, &CollectionsService::onCollectionLoading);

	this->setupEvent(
		obs::frontend::event::SCENE_COLLECTION_LOAD,
		&CollectionsService::onCollectionLoad
	);

	this->setupEvent(
		obs::frontend::event::SCENE_COLLECTION_CLEANUP,
		&CollectionsService::onCollectionCleaned
	);

	this->setupEvent(obs::frontend::event::EXIT, &CollectionsService::onExit);

	this->setupEvent(
		obs::frontend::event::SCENE_COLLECTION_LIST_CHANGED,
		&CollectionsService::onCollectionsListChanged
	);

	this->setupEvent(
		obs::frontend::event::SCENE_COLLECTION_CHANGED,
		&CollectionsService::onCollectionSwitched
	);

	this->setupEvent(
		rpc::event::FETCH_COLLECTIONS_SCHEMA,
		&CollectionsService::onFetchCollectionsSchema
	);

	this->setupEvent(rpc::event::GET_COLLECTIONS, &CollectionsService::onGetCollections);

	this->setupEvent(rpc::event::GET_ACTIVE_COLLECTION, &CollectionsService::onGetActiveCollection);

	this->setupEvent(rpc::event::MAKE_COLLECTION_ACTIVE, &CollectionsService::onMakeCollectionActive);

	this->setupEvent(
		rpc::event::COLLECTION_ADDED_SUBSCRIBE,
		&CollectionsService::subscribeCollectionChange
	);

	this->setupEvent(
		rpc::event::COLLECTION_REMOVED_SUBSCRIBE,
		&CollectionsService::subscribeCollectionChange
	);

	this->setupEvent(
		rpc::event::COLLECTION_UPDATED_SUBSCRIBE,
		&CollectionsService::subscribeCollectionChange
	);

	this->setupEvent(
		rpc::event::COLLECTION_SWITCHED_SUBSCRIBE,
		&CollectionsService::subscribeCollectionChange
	);
}

CollectionsService::~CollectionsService() {
}

/*
========================================================================================================
	OBS Event Handling
========================================================================================================
*/

bool
CollectionsService::onExit() {
	obsManager()->cleanRegisteredSourcesScenes();
	return true;
}

bool
CollectionsService::onCollectionLoad() {
	obsManager()->cleanRegisteredSourcesScenes();
	return true;
}

bool
CollectionsService::onCollectionCleaned() {
	auto p = [](void* private_data, obs_source_t* obs_source) -> bool {
		Q_UNUSED(private_data);
		const char* name = obs_source_get_name(obs_source);
		log_warn << QString("%1 exists always").arg(name).toStdString() << log_end;
		obs_source_release(obs_source);
		return true;
	};
	obs_enum_sources(p, nullptr);
	obsManager()->resetCollection();
	logInfo("Clean collection");
	return true;
}

bool
CollectionsService::onCollectionLoading(const obs::save::data& data) {

	Q_UNUSED(data);

	obsManager()->makeActive();

	obsManager()->activeCollection()->synchronize();

	// OBS Manager is loading collections, we don't do anything
	if(obsManager()->isLoadingCollection())
		return true;

	obsManager()->cleanRegisteredSourcesScenes();

	obsManager()->activeCollection()->switching = true;

	logInfo("Collection Loaded.");
	return true;
}

bool
CollectionsService::onCollectionsListChanged() {
	obs::collection::event evt = obsManager()->updateCollections(m_collectionUpdated);
	switch(evt) {
		case obs::collection::event::ADDED:
			return onCollectionAdded(*m_collectionUpdated.get());
			break;
		case obs::collection::event::REMOVED:
			return onCollectionRemoved(*m_collectionUpdated.get());
			break;
		case obs::collection::event::RENAMED:
			return onCollectionUpdated(*m_collectionUpdated.get());
			break;
	}
	return false;
}

bool
CollectionsService::onCollectionSwitched() {

	// OBS Manager is loading collections, we don't notify anything
	if(obsManager()->isLoadingCollection())
		return true;

	obsManager()->activeCollection()->switching = false;
	obsManager()->activeCollection()->makeActive();

	Collection* collection = obsManager()->activeCollection();
	logInfo(QString("Collection switched to %1.")
		.arg(collection->name().c_str())
		.toStdString()
	);

	obsManager()->registerAllSourcesScenes();

	if(m_collectionUpdated != nullptr) {
		m_collectionUpdated = nullptr;
		return true;
	}

	bool active = true;
	if(m_collectionToSwitch != 0x0) {
		rpc::response<void> response = response_void(nullptr, "onMakeCollectionActive");
		response.event = rpc::event::MAKE_COLLECTION_ACTIVE;
		m_collectionToSwitch = 0x0;
		active &= streamdeckManager()->commit_all(response, &StreamdeckManager::setAcknowledge);
	}

	rpc::response<CollectionPtr> response = response_collection(nullptr, "onCollectionSwitched");
	response.event = rpc::event::COLLECTION_SWITCHED_SUBSCRIBE;
	response.data = collection;

	return active && streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
CollectionsService::onCollectionAdded(const Collection& collection) {
	logInfo(QString("Collection %1 (%2) added.")
		.arg(collection.name().c_str())
		.arg(collection.id())
		.toStdString()
	);

	rpc::response<void> response = response_void(nullptr, "onCollectionAdded");
	response.event = rpc::event::COLLECTION_ADDED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
CollectionsService::onCollectionRemoved(const Collection& collection) {
	logInfo(QString("Collection %1 (%2) removed.")
		.arg(collection.name().c_str())
		.arg(collection.id())
		.toStdString()
	);

	rpc::response<void> response = response_void(nullptr, "onCollectionRemoved");
	response.event = rpc::event::COLLECTION_REMOVED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
CollectionsService::onCollectionUpdated(const Collection& collection) {
	logInfo(QString("Collection %1 (%2) renamed.")
		.arg(collection.name().c_str())
		.arg(collection.id())
		.toStdString()
	);

	rpc::response<CollectionPtr> response = response_collection(nullptr, "onCollectionUpdated");
	response.event = rpc::event::COLLECTION_UPDATED_SUBSCRIBE;
	response.data = const_cast<CollectionPtr>(&collection);

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

/*
========================================================================================================
	RPC Event Handling
========================================================================================================
*/

bool
CollectionsService::subscribeCollectionChange(const rpc::request& data) {
	rpc::response<std::string> response = response_string(&data, "subscribeCollectionChange");
	if(data.event == rpc::event::COLLECTION_ADDED_SUBSCRIBE ||
		data.event == rpc::event::COLLECTION_REMOVED_SUBSCRIBE || 
		data.event == rpc::event::COLLECTION_UPDATED_SUBSCRIBE ||
		data.event == rpc::event::COLLECTION_SWITCHED_SUBSCRIBE
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
CollectionsService::onFetchCollectionsSchema(const rpc::request& data) {
	rpc::response<std::string> response = response_string(&data, "onFetchCollectionsSchema");
	if(data.event == rpc::event::FETCH_COLLECTIONS_SCHEMA) {
		response.event = rpc::event::FETCH_COLLECTIONS_SCHEMA;
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

	rpc::response<Collections> response2 = response_collections(&data, "onFetchCollectionsSchema");
	response2.event = rpc::event::FETCH_COLLECTIONS_SCHEMA;
	response2.data = obsManager()->collections();

	return streamdeckManager()->commit_to(response2, &StreamdeckManager::setSchema);
}

bool
CollectionsService::onGetCollections(const rpc::request& data) {
	
	rpc::response<Collections> response = response_collections(&data, "onGetCollections");
	if(data.event == rpc::event::GET_COLLECTIONS) {
		response.event = rpc::event::GET_COLLECTIONS;
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
CollectionsService::onGetActiveCollection(const rpc::request& data) {
	rpc::response<CollectionPtr> response = response_collection(&data, "onGetActiveCollection");
	if(data.event == rpc::event::GET_ACTIVE_COLLECTION) {
		response.event = rpc::event::GET_ACTIVE_COLLECTION;
		logInfo("Active Collection required.");

		if(!checkResource(&data, QRegExp("activeCollection"))) {
			logWarning("Unknown resource for activeCollection.");
		}

		response.data = obsManager()->activeCollection();
		return streamdeckManager()->commit_to(response, &StreamdeckManager::setCollection);
	}

	logError("GetActiveCollection not called by GET_ACTIVE_COLLECTION.");
	return false;
}

bool
CollectionsService::onMakeCollectionActive(const rpc::request& data) {
	rpc::response<void> response = response_void(&data, "onMakeCollectionActive");
	if(data.event == rpc::event::MAKE_COLLECTION_ACTIVE) {
		response.event = rpc::event::MAKE_COLLECTION_ACTIVE;

		if(!checkResource(&data, QRegExp("load"))) {
			logWarning("Unknown resource for makeCollectionActive.");
		}

		if(data.args.size() == 0) {
			logError("No parameter provided for makeCollectionActive. Abort.");
			return false;
		}

		m_collectionToSwitch = data.args[0].toString().toShort();
		if(!obsManager()->switchCollection(m_collectionToSwitch)) {
			logError("The required collection doesn't exist, or can't be switched to.");
			m_collectionToSwitch = 0x0;
			return false;
		}

		// Acknowledge is sent when collection has been switched
		return true;

		//return streamdeckManager()->commit_to(response, &StreamdeckManager::setAcknowledge);
	}

	logError("MakeCollectionActive not called by MAKE_COLLECTION_ACTIVE.");
	return false;
}