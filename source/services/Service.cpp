/*
 * Plugin Includes
 */
#include "include/services/Service.hpp"
#include "include/common/Logger.hpp"

/*
========================================================================================================
	Static Class Attributes Initialization
========================================================================================================
*/

StreamdeckManager* Service::_streamdeck_manager = nullptr;

OBSManager* Service::_obs_manager = nullptr;

bool Service::_obs_started = false;

/*
========================================================================================================
	OBS Event Handling
========================================================================================================
*/

void
Service::OnOBSFrontendEvent(obs_frontend_event event, void* service) {
	_obs_started = _obs_started | event == OBS_FRONTEND_EVENT_FINISHED_LOADING;
	reinterpret_cast<Service*>(service)->m_frontendEvent.notifyEvent(event);
}

void
Service::OnOBSSaveEvent(obs_data_t* save_data, bool saving, void* service) {
	obs_save_event event = saving ?
		obs_save_event::OBS_SAVE_EVENT_SAVING : obs_save_event::OBS_SAVE_EVENT_LOADING;
	reinterpret_cast<Service*>(service)->m_saveEvent.notifyEvent<const obs_data_t*>(event, save_data);
}


/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Service::Service(const char* local_name, const char* remote_name) : 
	m_localName(local_name),
	m_remoteName(remote_name) {
	obs_frontend_add_event_callback(Service::OnOBSFrontendEvent, this);
	obs_frontend_add_save_callback(Service::OnOBSSaveEvent, this);
}

Service::~Service() {
	obs_frontend_remove_event_callback(Service::OnOBSFrontendEvent, this);
	obs_frontend_remove_save_callback(Service::OnOBSSaveEvent, this);
}