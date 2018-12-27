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
	Macros Definitions
========================================================================================================
*/

#define OBS_OUTPUT_STATE(E, T) \
	m_states[T] = boost::bind(&OutputEventTrigger::outputState, this, E, _1, T)

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class OutputEventTrigger : public EventTrigger<OutputEventTrigger, obs::output::event> {

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	private:

		static void
		OnOuputEvent(void* trigger_data, calldata_t* data) {
			if(!Service::_obs_started) return;

			typedef std::function<void(obs_output_t*)> state;
			state func = *reinterpret_cast<state*>(trigger_data);

			obs_output_t* data_output = nullptr;
			calldata_get_ptr(data, "output", &data_output);

			func(data_output);
		}

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::map<const char*, std::function<void(obs_output_t*)>> m_states;

		std::set<obs_output_t*> m_outputs;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		OutputEventTrigger() :
			EventTrigger<OutputEventTrigger, obs::output::event>() {
			OBS_OUTPUT_STATE(obs::output::event::STARTING, "starting");
			OBS_OUTPUT_STATE(obs::output::event::STARTED, "start");
			OBS_OUTPUT_STATE(obs::output::event::STOPPING, "stopping");
			OBS_OUTPUT_STATE(obs::output::event::STOPPED, "stop");
			OBS_OUTPUT_STATE(obs::output::event::RECONNECTING, "reconnect");
			OBS_OUTPUT_STATE(obs::output::event::RECONNECTED, "reconnect_success");
		}

		~OutputEventTrigger() {
			for(auto iter = m_outputs.begin(); iter != m_outputs.end();) {
				auto remove = iter;
				iter++;
				removeOutput(*remove);
			}
		}

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void
		addOutput(obs_output_t* output) {
			if(m_outputs.find(output) == m_outputs.end()) {
				signal_handler_t* signal_handler = obs_output_get_signal_handler(output);
				if(signal_handler != nullptr) {
					for(auto iter = m_states.begin(); iter != m_states.end(); iter++) {
						signal_handler_connect(
							signal_handler,
							iter->first,
							OutputEventTrigger::OnOuputEvent,
							&iter->second
						);
					}
					m_outputs.insert(output);
				}
			}
		}

		void
		removeOutput(obs_output_t* output) {
			if(m_outputs.find(output) != m_outputs.end()) {
				signal_handler_t* signal_handler = obs_output_get_signal_handler(output);
				if(signal_handler != nullptr) {
					for(auto iter = m_states.begin(); iter != m_states.end(); iter++) {
						signal_handler_disconnect(
							signal_handler,
							iter->first,
							OutputEventTrigger::OnOuputEvent,
							&iter->second
						);
					}
					m_outputs.erase(output);
				}
			}
		}

	private:

		void outputState(obs::output::event event, obs_output_t* output, const char* state) {
			if(m_outputs.find(output) == m_outputs.end()) return;
			obs::output::data obs_output_data = obs::output::data{ event, output, state };

			if(Service::_obs_started) {
				m_event.notifyEvent<const obs::output::data&>(event, obs_output_data);
			}
		}

};