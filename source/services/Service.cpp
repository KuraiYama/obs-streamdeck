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

/*
========================================================================================================
	OBS Event Handling
========================================================================================================
*/

void
Service::OnObsFrontendEvent(obs_frontend_event event, void* service) {
	reinterpret_cast<Service*>(service)->m_frontendEvent.notifyEvent(event);
}

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Service::Service(const char* local_name, const char* remote_name) : 
	m_localName(local_name),
	m_remoteName(remote_name) {
	obs_frontend_add_event_callback(Service::OnObsFrontendEvent, this);
}

Service::~Service() {
	obs_frontend_remove_event_callback(Service::OnObsFrontendEvent, this);
}