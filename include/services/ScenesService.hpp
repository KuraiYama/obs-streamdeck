#pragma once

/*
 * Qt Includes
 */
#include <QMap>
#include <QMainWindow>
#include <QAction>

/*
 * OBS Includes
 */
#include <obs.h>
#include <obs-frontend-api/obs-frontend-api.h>
#include <obs-module.h>

/*
 * Plugin Includes
 */
#include "include/services/Service.hpp"
#include "include/streamdeck/StreamdeckManager.hpp"
#include "include/obs/Collection.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class ScenesService : public ServiceImpl<ScenesService> {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::shared_ptr<Scene> m_sceneUpdated;

		uint16_t m_sceneToSwitch;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		ScenesService();

		virtual ~ScenesService();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	private:

		bool
		onScenesListChanged();

		bool
		subscribeSceneChange(const rpc::request& data);

		bool
		onSceneAdded(const Scene& scene);

		bool
		onSceneRemoved(const Scene& scene);

		bool
		onSceneUpdated(const Scene& scene);

		bool
		onSceneSwitched();

		bool
		onGetScenes(const rpc::request& data);

		bool
		onGetActiveScene(const rpc::request& data);

		bool
		onMakeSceneActive(const rpc::request& data);

};