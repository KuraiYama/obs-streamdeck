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
CollectionManager* collectionManager;

/*
========================================================================================================
	Module Functions
========================================================================================================
*/

bool
obs_module_load(void) {
	QMainWindow *parent = (QMainWindow*)obs_frontend_get_main_window();

	streamdeckManager = new StreamdeckManager(parent);
	collectionManager = new CollectionManager();

	services.push_back((Service*)new ApplicationService(parent, streamdeckManager, collectionManager));
	//services.push_back((Service*)new StreamingService(streamdeckManager));
	//services.push_back((Service*)new RecordingService(streamdeckManager));
	//services.push_back((Service*)new CollectionsService(streamdeckManager, collectionManager));
	//services.push_back((Service*)new ScenesService(streamdeckManager, collectionManager));

	return true;
}

void
obs_module_unload(void) {

	delete streamdeckManager;
	delete collectionManager;

	for(auto i = services.begin(); i != services.end(); i++)
		delete *i;
	services.clear();
}

/*
========================================================================================================
	Utility Function
========================================================================================================
*/

std::string
ptr_to_string(void* ptr) {
	return std::to_string(reinterpret_cast<unsigned long long>(ptr));
}

void*
string_to_ptr(const char* str) {
	return reinterpret_cast<void*>(std::atoll(str));
}
