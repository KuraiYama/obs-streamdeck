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

ScenesService::ScenesService() : 
	ServiceT("ScenesService", "ScenesService") {

	this->setupEvent(obs_frontend_event::OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED,
		&ScenesService::onScenesListChanged);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::SCENE_ADDED_SUBSCRIBE,
		&ScenesService::subscribeSceneChange);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::SCENE_REMOVED_SUBSCRIBE,
		&ScenesService::subscribeSceneChange);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::SCENE_SWITCHED_SUBSCRIBE,
		&ScenesService::subscribeSceneChange);

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

bool
ScenesService::onScenesListChanged() {
	std::shared_ptr<Scene> scene_updated = nullptr;
	obs::scene_event evt = obsManager()->updateScenes(*obsManager()->activeCollection(), scene_updated);
	switch(evt) {
		case obs::scene_event::SCENE_ADDED:
			onSceneAdded(*scene_updated.get());
			break;
		case obs::scene_event::SCENE_REMOVED:
			onSceneRemoved(*scene_updated.get());
			break;
		case obs::scene_event::SCENE_RENAMED:
			onSceneUpdated(*scene_updated.get());
			break;
	}
	return true;
}

/*bool
ScenesService::onSceneSwitched() {
}*/

bool
ScenesService::onSceneAdded(const Scene& scene) {
	logInfo(QString("Scene %1 added").arg(scene.name().c_str()).toStdString());
}

bool
ScenesService::onSceneRemoved(const Scene& scene) {
	logInfo(QString("Scene %1 removed").arg(scene.name().c_str()).toStdString());
}

bool
ScenesService::onSceneUpdated(const Scene& scene) {
	logInfo(QString("Scene renamed to %1").arg(scene.name().c_str()).toStdString());
}

/*
========================================================================================================
	RPC Event Handling
========================================================================================================
*/

bool
ScenesService::subscribeSceneChange(const rpc_event_data& data) {
	rpc_adv_response<std::string> response = response_string(&data, "subscribeSceneChange");
	if(data.event == Streamdeck::rpc_event::SCENE_ADDED_SUBSCRIBE ||
		data.event == Streamdeck::rpc_event::SCENE_REMOVED_SUBSCRIBE ||
		data.event == Streamdeck::rpc_event::SCENE_SWITCHED_SUBSCRIBE
	) {
		response.event = data.event;
		logInfo("Subscription to scene event required");

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

	logError("subscribeSceneChange not called by SCENE_SUBSCRIBE");
	return false;
}

/*bool
ScenesService::onGetScenes(const rpc_event_data& data) {

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
}*/
