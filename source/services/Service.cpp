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
	Constructors / Destructor
========================================================================================================
*/

Service::Service(const char* local_name, const char* remote_name) : 
	m_localName(local_name),
	m_remoteName(remote_name) {
}

Service::~Service() {
}