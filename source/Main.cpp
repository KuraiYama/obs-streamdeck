/*
 * CRT Includes
 */
#include <cstdlib>

/*
 * Qt Includes
 */
#include <QMainWindow>
#include <QAction>

/*
 * Boost Includes
 */
#include <boost/bind.hpp>
#include <boost/function.hpp>

/*
 * OBS Includes
 */
#include <obs.h>
#include <obs-frontend-api/obs-frontend-api.h>
#include <obs-module.h>

/*
 * Plugin Includes
 */
#include "include/streamdeck/StreamDeckManager.hpp"
#include "include/obs/Collection.hpp"
#include "include/services/ApplicationService.hpp"
#include "include/services/StreamingService.hpp"
#include "include/services/RecordingService.hpp"
#include "include/services/CollectionsService.hpp"
#include "include/services/ScenesService.hpp"

/*
========================================================================================================
	OBS Module Declarations
========================================================================================================
*/

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("frontend-tools", "en-US")

/*
========================================================================================================
	Global Variables
========================================================================================================
*/

QVector<Service*> services;
StreamdeckManager* streamdeckManager;
OBSManager* collectionManager;

/*
========================================================================================================
	Module Functions
========================================================================================================
*/

bool
obs_module_load(void) {
	QMainWindow *parent = (QMainWindow*)obs_frontend_get_main_window();

	Service::_streamdeck_manager = new StreamdeckManager();
	Service::_obs_manager = new OBSManager();

	services.push_back(new ApplicationService(parent, "database.dat"));
	services.push_back(new StreamingService());
	services.push_back(new RecordingService());
	services.push_back(new CollectionsService());
	services.push_back(new ScenesService());

	return true;
}

void
obs_module_unload(void) {
	delete Service::_streamdeck_manager;
	delete Service::_obs_manager;

	for(auto i = services.begin(); i != services.end(); i++)
		delete *i;

	services.clear();
}