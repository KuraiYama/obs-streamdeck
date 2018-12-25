#pragma once

/*
 * OBS Includes
 */
#include <obs.h>
#include <obs-frontend-api/obs-frontend-api.h>

/*
 * Plugin Includes
 */
#include "include/events/EventTrigger.hpp"
#include "include/services/Service.hpp"
#include "include/obs/OBSEvents.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class SaveEventTrigger : public EventTrigger<SaveEventTrigger, obs::save::event> {

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	private:

		static void
		OnSaveEvent(obs_data_t* save_data, bool saving, void* trigger) {
			if(!Service::_obs_started) return;
			obs::save::event event = saving ? obs::save::event::SAVING : obs::save::event::LOADING;
			notify<const obs::save::data&>(trigger, event, { event, save_data });
		}

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		SaveEventTrigger() :
			EventTrigger<SaveEventTrigger, obs::save::event>() {
			obs_frontend_add_save_callback(SaveEventTrigger::OnSaveEvent, this);
		}

		~SaveEventTrigger() {
			obs_frontend_remove_save_callback(SaveEventTrigger::OnSaveEvent, this);
		}

};