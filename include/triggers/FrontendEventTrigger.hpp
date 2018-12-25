#pragma once

/*
 * OBS Includes
 */
#include <obs.h>
#include <obs-frontend-api/obs-frontend-api.h>

/*
 * Plugin Includes
 */
#include "include/services/Service.hpp"
#include "include/events/EventTrigger.hpp"
#include "include/obs/OBSEvents.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class FrontendEventTrigger : public EventTrigger<FrontendEventTrigger, obs::frontend::event> {

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	private:

		static void
		OnFrontendEvent(obs_frontend_event event, void* trigger) {
			Service::_obs_started =
				Service::_obs_started || event == OBS_FRONTEND_EVENT_FINISHED_LOADING;

			if(Service::_obs_started) {
				notify(trigger, static_cast<obs::frontend::event>(event));
			}
		}

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		FrontendEventTrigger() :
			EventTrigger<FrontendEventTrigger, obs::frontend::event>() {
			obs_frontend_add_event_callback(FrontendEventTrigger::OnFrontendEvent, this);
		}

		~FrontendEventTrigger() {
			obs_frontend_remove_event_callback(FrontendEventTrigger::OnFrontendEvent, this);
		}

};