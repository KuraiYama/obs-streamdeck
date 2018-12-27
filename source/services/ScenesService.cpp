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
	ServiceImpl("ScenesService", "ScenesService"),
	m_sceneUpdated(nullptr),
	m_sceneToSwitch(0x0) {

	this->setupEvent(obs::frontend::event::SCENE_LIST_CHANGED, &ScenesService::onScenesListChanged);

	this->setupEvent(obs::frontend::event::SCENE_CHANGED, &ScenesService::onSceneSwitched);

	this->setupEvent(rpc::event::SCENE_ADDED_SUBSCRIBE, &ScenesService::subscribeSceneChange);

	this->setupEvent(rpc::event::SCENE_REMOVED_SUBSCRIBE, &ScenesService::subscribeSceneChange);

	this->setupEvent(rpc::event::SCENE_SWITCHED_SUBSCRIBE, &ScenesService::subscribeSceneChange);

	this->setupEvent(rpc::event::GET_SCENES, &ScenesService::onGetScenes);

	this->setupEvent(rpc::event::GET_ACTIVE_SCENE, &ScenesService::onGetActiveScene);

	this->setupEvent(rpc::event::MAKE_SCENE_ACTIVE, &ScenesService::onMakeSceneActive);
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
	obs::scene::event evt = obsManager()->activeCollection()->updateScenes(m_sceneUpdated);
	switch(evt) {
	case obs::scene::event::ADDED:
		return onSceneAdded(*m_sceneUpdated.get());
		break;
	case obs::scene::event::REMOVED:
		return onSceneRemoved(*m_sceneUpdated.get());
		break;
	case obs::scene::event::RENAMED:
		return onSceneUpdated(*m_sceneUpdated.get());
		break;
	default:
		break;
	}
	return true;
}

bool
ScenesService::onSceneSwitched() {

	// We are loading or switching collections - we don't notify the switch scene
	if(obsManager()->isLoadingCollection() || obsManager()->activeCollection()->switching)
		return true;

	obsManager()->activeCollection()->makeActive();
	Scene* scene = obsManager()->activeCollection()->activeScene();
	logInfo(QString("Scene switched to %1.")
		.arg(scene->name().c_str())
		.toStdString()
	);

	/*if(m_sceneUpdated != nullptr) {
		m_sceneUpdated = nullptr;
		return true;
	}*/

	bool activ = true;
	if(m_sceneToSwitch != 0x0) {
		rpc::response<bool> response_switch = response_bool(nullptr, "onSceneSwitched");
		response_switch.event = rpc::event::MAKE_SCENE_ACTIVE;
		response_switch.data = m_sceneToSwitch == obsManager()->activeCollection()->activeScene()->id();
		m_sceneToSwitch = 0x0;
		activ &= streamdeckManager()->commit_all(response_switch, &StreamdeckManager::setResult);
	}

	rpc::response<ScenePtr> response = response_scene(nullptr, "onSceneSwitched");
	response.event = rpc::event::SCENE_SWITCHED_SUBSCRIBE;
	response.data = scene;

	return activ && streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
ScenesService::onSceneAdded(const Scene& scene) {
	logInfo(QString("Scene %1 (%2) added.")
		.arg(scene.name().c_str())
		.arg(scene.id())
		.toStdString()
	);

	obsManager()->registerScene(&scene);

	rpc::response<void> response = response_void(nullptr, "onSceneAdded");
	response.event = rpc::event::SCENE_ADDED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
ScenesService::onSceneRemoved(const Scene& scene) {
	logInfo(QString("Scene %1 (%2) removed.")
		.arg(scene.name().c_str())
		.arg(scene.id())
		.toStdString()
	);

	obsManager()->unregisterScene(&scene);

	rpc::response<void> response = response_void(nullptr, "onSceneRemoved");
	response.event = rpc::event::SCENE_REMOVED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
ScenesService::onSceneUpdated(const Scene& scene) {
	logInfo(QString("Scene renamed to %1").arg(scene.name().c_str()).toStdString());

	// The RPC protocol doesn't provide any resource for handling scene renaming.
	// We can use both scene removed/scene added to handle that, but each of them
	// implies GET_SCENES message. Then we send directly the GET_SCENES message instead.

	rpc::response<Scenes> response = response_scenes(nullptr, "onSceneUpdated");
	response.event = rpc::event::GET_SCENES;
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
ScenesService::subscribeSceneChange(const rpc::request& data) {
	rpc::response<std::string> response = response_string(&data, "subscribeSceneChange");
	if(data.event == rpc::event::SCENE_ADDED_SUBSCRIBE ||
		data.event == rpc::event::SCENE_REMOVED_SUBSCRIBE ||
		data.event == rpc::event::SCENE_SWITCHED_SUBSCRIBE
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
ScenesService::onGetScenes(const rpc::request& data) {

	rpc::response<Scenes> response = response_scenes(&data, "onGetScenes");

	if(data.event == rpc::event::GET_SCENES) {
		response.event = rpc::event::GET_SCENES;
		logInfo("Scenes list required.");

		if(!checkResource(&data, QRegExp("getScenes"))) {
			logWarning("Unknown resource for getScenes.");
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
			uint16_t id = QString(data.args[0].toString()).toShort();
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
ScenesService::onGetActiveScene(const rpc::request& data) {
	rpc::response<ScenePtr> response = response_scene(&data, "onGetActiveScene");
	if(data.event == rpc::event::GET_ACTIVE_SCENE) {
		response.event = rpc::event::GET_ACTIVE_SCENE;
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
ScenesService::onMakeSceneActive(const rpc::request& data) {
	rpc::response<bool> response = response_bool(&data, "onMakeSceneActive");

	if(data.event == rpc::event::MAKE_SCENE_ACTIVE) {
		response.event = rpc::event::MAKE_SCENE_ACTIVE;

		if(!checkResource(&data, QRegExp("makeSceneActive"))) {
			logWarning("Unknown resource for makeSceneActive.");
		}

		if(data.args.size() == 0) {
			logError("No parameter provided for makeSceneActive. Abort.");
			return false;
		}

		uint32_t id = data.args[0].toString().toInt();
		uint16_t collection_id = (id & 0xFFFF0000) >> 16;
		uint16_t scene_id = id & 0x0000FFFF;

		if(collection_id != obsManager()->activeCollection()->id()) {
			logWarning("Scene doesn't belong to the active collection. Abort.");
		}

		response.data = scene_id == obsManager()->activeCollection()->activeScene()->id();

		// Return when switched is finished
		if(!response.data && collection_id == obsManager()->activeCollection()->id() &&
			obsManager()->activeCollection()->switchScene(scene_id)) {
			m_sceneToSwitch = scene_id;
			return true;
		}
		else if(response.data) {
			rpc::response<ScenePtr> response_switch = response_scene(nullptr, "onSceneSwitched");
			response_switch.event = rpc::event::SCENE_SWITCHED_SUBSCRIBE;
			response_switch.data = obsManager()->activeCollection()->activeScene();

			return streamdeckManager()->commit_to(response, &StreamdeckManager::setResult) &&
				streamdeckManager()->commit_all(response_switch, &StreamdeckManager::setEvent);
		}
		
		return streamdeckManager()->commit_to(response, &StreamdeckManager::setResult);
	}

	logError("MakeSceneActive not called by MAKE_SCENE_ACTIVE.");
	return false;
}