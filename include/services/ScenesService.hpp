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

class ScenesService : public ServiceT<ScenesService> {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		CollectionManager* m_collectionManager;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		ScenesService(StreamdeckManager* streamdeckManager, CollectionManager* collectionManager);

		virtual ~ScenesService();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	private:

		bool onGetScenes(const rpc_event_data& data);

};