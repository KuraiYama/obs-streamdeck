/*
 * Plugin Includes
 */
#include "include/services/ScenesService.hpp"
#include "include/common/Logger.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

ScenesService::ScenesService(StreamdeckManager* streamdeckManager, CollectionManager* collectionManager) : 
		ServiceT("ScenesService", streamdeckManager) {

	m_collectionManager = collectionManager;

	/*this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::RPC_ID_GET_SCENES,
		&ScenesService::onGetScenes);*/
}

ScenesService::~ScenesService() {
}

/*
========================================================================================================
	OBS Event Handling
========================================================================================================
*/

/*bool ScenesService::onScenesListChanged() {
	CollectionManager::obs_collection_event evt = m_collectionManager->buildCollections();

	switch(evt) {
	case CollectionManager::obs_collection_event::OBS_COLLECTION_EVENT_ADDED:
		return onSceneAdded();
		break;

	case CollectionManager::obs_collection_event::OBS_COLLECTION_EVENT_REMOVED:
		return onSceneRemoved();
		break;

	case CollectionManager::obs_collection_event::OBS_COLLECTION_EVENT_UPDATED:
		return onSceneUpdated();
		break;

	default:
		break;
	}

	return false;
}

bool ScenesService::onSceneSwitched() {
}

bool ScenesService::onSceneAdded() {
}

bool ScenesService::onSceneRemoved() {
}

bool ScenesService::onSceneUpdated() {
}*/

/*
========================================================================================================
	RPC Event Handling
========================================================================================================
*/

bool ScenesService::onGetScenes(const rpc_event_data& data) {

	rpc_adv_response<std::tuple<Collection*,Scenes>> response = response_scenes(&data, "onGetScenes");

	if(data.event == Streamdeck::rpc_event::RPC_ID_GET_SCENES) {
		response.event = Streamdeck::rpc_event::RPC_ID_GET_SCENES;
		if(data.serviceName.compare("ScenesService") == 0 && data.method.compare("getScenes") == 0) {
			
			if(!data.args.empty() && data.args[0].canConvert<QString>()) {
				QString collection = data.args[0].toString();
				Collection* collection_ptr = nullptr;
				if(collection.compare("") == 0) {
					collection_ptr = m_collectionManager->activeCollection();
				}
				else {
					collection_ptr = m_collectionManager->getCollectionByName(collection.toStdString());
				}

				if(collection_ptr != nullptr) {
					response.data = std::tuple<Collection*, Scenes>(collection_ptr, 
						collection_ptr->scenes());
				}

				return streamdeckManager()->commit_to(response, &StreamdeckManager::setScenes);
			}

		}
	}

	return false;
}
