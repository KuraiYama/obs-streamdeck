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

	this->setupEvent(obs_frontend_event::OBS_FRONTEND_EVENT_SCENE_CHANGED,
		&ScenesService::onSceneSwitched);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::SCENE_ADDED_SUBSCRIBE,
		&ScenesService::subscribeSceneChange);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::SCENE_REMOVED_SUBSCRIBE,
		&ScenesService::subscribeSceneChange);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::SCENE_SWITCHED_SUBSCRIBE,
		&ScenesService::subscribeSceneChange);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::GET_SCENES,
		&ScenesService::onGetScenes);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::GET_ACTIVE_SCENE,
		&ScenesService::onGetActiveScene);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::MAKE_SCENE_ACTIVE,
		&ScenesService::onMakeSceneActive);
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
			return onSceneAdded(*scene_updated.get());
			break;
		case obs::scene_event::SCENE_REMOVED:
			return onSceneRemoved(*scene_updated.get());
			break;
		case obs::scene_event::SCENE_RENAMED:
			return onSceneUpdated(*scene_updated.get());
			break;
	}
	return true;
}

bool
ScenesService::onSceneSwitched() {
	// During loading, we don't send anything to the streamdecks
	if(obsManager()->isLoadingCollections())
		return true;

	Collection* current_collection = obsManager()->activeCollection();

	// No switch when current collection is not computed
	if(current_collection == nullptr)
		return true;

	Scene* scene = current_collection->activeScene();
	logInfo(QString("Scene switched to %1.")
		.arg(scene->name().c_str())
		.toStdString()
	);

	rpc_adv_response<ScenePtr> response = response_scene(nullptr, "onSceneSwitched");
	response.event = Streamdeck::rpc_event::SCENE_SWITCHED_SUBSCRIBE;
	response.data = scene;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
ScenesService::onSceneAdded(const Scene& scene) {
	logInfo(QString("Scene %1 (%2) added.")
		.arg(scene.name().c_str())
		.arg(scene.id())
		.toStdString()
	);

	rpc_adv_response<void> response = response_void(nullptr, "onSceneAdded");
	response.event = Streamdeck::rpc_event::SCENE_ADDED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
ScenesService::onSceneRemoved(const Scene& scene) {
	logInfo(QString("Scene %1 (%2) removed.")
		.arg(scene.name().c_str())
		.arg(scene.id())
		.toStdString()
	);

	rpc_adv_response<void> response = response_void(nullptr, "onSceneRemoved");
	response.event = Streamdeck::rpc_event::SCENE_REMOVED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
ScenesService::onSceneUpdated(const Scene& scene) {
	logInfo(QString("Scene renamed to %1").arg(scene.name().c_str()).toStdString());

	// The RPC protocol doesn't provide any resource for handling scene renaming.
	// We can use both scene removed/scene added to handle that, but each of them
	// implies GET_SCENES message. Then we send directly the GET_SCENES message instead.

	rpc_adv_response<Scenes> response = response_scenes(nullptr, "onSceneUpdated");
	response.event = Streamdeck::rpc_event::GET_SCENES;
#if !defined(COMPLETE_MODE)
	Collection* collection = scene.collection();
	response.data = collection->scenes();

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setScenes);
#else
	Collections collections = obsManager()->collections();
	bool result = true;
	for(auto iter = collections.begin(); iter < collections.end() && result; iter++) {
		response.data = (*iter)->scenes();
		result &= streamdeckManager()->commit_all(response, &StreamdeckManager::setScenes);
	}
	return result;
#endif
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

bool
ScenesService::onGetScenes(const rpc_event_data& data) {

	rpc_adv_response<Scenes> response = response_scenes(&data, "onGetScenes");

	if(data.event == Streamdeck::rpc_event::GET_SCENES) {
		response.event = Streamdeck::rpc_event::GET_SCENES;
		logInfo("Scenes list required.");

		if(!checkResource(&data, QRegExp("getScenes"))) {
			logWarning("Unknown resource for activeCollection.");
		}

		if(data.args.size() <= 0) {
			logError("No argument provided by get_scenes.");
			return false;
		}

		Collection* collection = nullptr;
		if(data.args[0].compare("") == 0) {
#if defined(COMPLETE_MODE)
			Collections collections = obsManager()->collections();
			bool result = true;
			for(auto iter = collections.begin(); iter < collections.end() && result; iter++) {
				response.data = (*iter)->scenes();
				result &= streamdeckManager()->commit_to(response, &StreamdeckManager::setScenes);
			}
			return result;
#else
			collection = obsManager()->activeCollection();
#endif
		}
		else {
			unsigned long long id = QString(data.args[0].toString()).toLongLong();
			collection = obsManager()->collection(id);
		}

		if(collection != nullptr)
			response.data = collection->scenes();

		return streamdeckManager()->commit_to(response, &StreamdeckManager::setScenes);
	}

	logError("getScenes not called by GET_SCENES");
	return false;
}

bool
ScenesService::onGetActiveScene(const rpc_event_data& data) {
	rpc_adv_response<ScenePtr> response = response_scene(&data, "onGetActiveScene");
	if(data.event == Streamdeck::rpc_event::GET_ACTIVE_SCENE) {
		response.event = Streamdeck::rpc_event::GET_ACTIVE_SCENE;
		logInfo("Active Scene required.");

		if(!checkResource(&data, QRegExp("activeSceneId"))) {
			logWarning("Unknown resource for activeScene.");
		}

		response.data = obsManager()->activeCollection()->activeScene();

		return streamdeckManager()->commit_to(response, &StreamdeckManager::setScene);
	}

	logError("GetActiveScene not called by GET_ACTIVE_SCENE.");
	return false;
}

bool
ScenesService::onMakeSceneActive(const rpc_event_data& data) {
	rpc_adv_response<bool> response = response_bool(&data, "onMakeSceneActive");
	if(data.event == Streamdeck::rpc_event::MAKE_SCENE_ACTIVE) {
		response.event = Streamdeck::rpc_event::MAKE_SCENE_ACTIVE;

		if(!checkResource(&data, QRegExp("makeSceneActive"))) {
			logWarning("Unknown resource for makeSceneActive.");
		}

		if(data.args.size() == 0) {
			logError("No parameter provided for makeSceneActive. Abort.");
			return false;
		}

		unsigned long long id = data.args[0].toString().toLongLong();
		response.data = obsManager()->activeCollection()->activeScene(id);

		return streamdeckManager()->commit_to(response, &StreamdeckManager::setResult);
	}

	logError("MakeSceneActive not called by MAKE_SCENE_ACTIVE.");
	return false;
}