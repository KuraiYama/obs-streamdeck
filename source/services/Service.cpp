/*
 * Plugin Includes
 */
#include "include/services/Service.hpp"
#include "include/common/Logger.hpp"

/*
========================================================================================================
	OBS Event Handling
========================================================================================================
*/

void
Service::OnObsFrontendEvent(obs_frontend_event event, void* service) {
	reinterpret_cast<Service*>(service)->m_frontendEvent.notifyEvent(event);
}

void
Service::OnObsSaveEvent(obs_data_t* save_data, bool saving, void* service) {
	obs_save_event event = saving ? 
		obs_save_event::OBS_SAVE_EVENT_SAVED : obs_save_event::OBS_SAVE_EVENT_LOADED;
	reinterpret_cast<Service*>(service)->m_saveEvent.notifyEvent<const obs_data_t*>(event, save_data);
}

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Service::Service(const char* name, StreamdeckManager* streamdeckManager) : 
		m_name(name), m_streamdeckManager(streamdeckManager) {
	obs_frontend_add_event_callback(Service::OnObsFrontendEvent, this);
	obs_frontend_add_save_callback(Service::OnObsSaveEvent, this);
}

Service::~Service() {
	obs_frontend_remove_event_callback(Service::OnObsFrontendEvent, this);
	obs_frontend_remove_save_callback(Service::OnObsSaveEvent, this);
}

/*
========================================================================================================
	Logging
========================================================================================================
*/

void
Service::logger(const std::string& message) const {
	log_info << QString("[%1] %2")
		.arg(QString(m_name))
		.arg(QString::fromStdString(message)).toStdString();
}