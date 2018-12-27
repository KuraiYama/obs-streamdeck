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

class SourceEventTrigger : public EventTrigger<SourceEventTrigger, obs::source::event> {

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	private:

		static void
		OnSourceCreated(void* trigger, calldata_t* data) {
			if(!Service::_obs_started) return;

			obs_source_t* source = nullptr;
			if(calldata_get_ptr(data, "source", &source)) {
				obs::source::data event_data = obs::source::data{ obs::source::event::ADDED, nullptr };
				event_data.data.obs_source = source;
				notify<const obs::source::data&>(trigger, obs::source::event::ADDED, event_data);
			}
		}

		static void
		OnSourceDestroyed(void* trigger, calldata_t* data) {
			if(!Service::_obs_started) return;

			obs_source_t* source = nullptr;
			if(calldata_get_ptr(data, "source", &source)) {
				auto trigger_typed = reinterpret_cast<SourceEventTrigger*>(trigger);
				auto source_it = trigger_typed->m_sources.find(source);
				if(source_it != trigger_typed->m_sources.end()) {
					obs::source::data event_data = obs::source::data{
						obs::source::event::REMOVED,
						source_it->second
					};
					trigger_typed->m_event.notifyEvent<const obs::source::data&>(
						obs::source::event::REMOVED,
						event_data
					);
				}
			}
		}

		static void
		Call(void* callback, calldata_t* data) {
			if(!Service::_obs_started) return;

			obs_source_t* source = nullptr;
			if(calldata_get_ptr(data, "source", &source)) {
				typedef boost::function<void(obs_source_t*, calldata_t*)> func;
				func function = *reinterpret_cast<func*>(callback);
				function(source, data);
			}
		}

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::map<const char*, boost::function<void(obs_source_t*, calldata_t*)>> m_callbacks;

		std::map<obs_source_t*, Source*> m_sources;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		SourceEventTrigger() :
			EventTrigger<SourceEventTrigger, obs::source::event>() {

			registerCallbacks();

			signal_handler_t* signal_handler = obs_get_signal_handler();
			if(signal_handler != nullptr) {
				signal_handler_connect(
					signal_handler,
					"source_create",
					SourceEventTrigger::OnSourceCreated,
					this
				);

				signal_handler_connect(
					signal_handler,
					"source_remove",
					SourceEventTrigger::OnSourceDestroyed,
					this
				);

				signal_handler_connect(
					signal_handler,
					"source_destroy",
					SourceEventTrigger::OnSourceDestroyed,
					this
				);
			}
		}

		~SourceEventTrigger() {

			removeAll();

			signal_handler_t* signal_handler = obs_get_signal_handler();
			if(signal_handler != nullptr) {
				signal_handler_disconnect(
					signal_handler,
					"source_create",
					SourceEventTrigger::OnSourceCreated,
					this
				);

				signal_handler_disconnect(
					signal_handler,
					"source_remove",
					SourceEventTrigger::OnSourceDestroyed,
					this
				);

				signal_handler_disconnect(
					signal_handler,
					"source_destroy",
					SourceEventTrigger::OnSourceDestroyed,
					this
				);
			}
		}

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void
		addSource(const Source* source) {
			if(m_sources.find(source->source()) == m_sources.end()) {
				signal_handler_t* signal_handler = obs_source_get_signal_handler(source->source());
				if(signal_handler != nullptr) {
					for(auto cb_iter = m_callbacks.begin(); cb_iter != m_callbacks.end(); cb_iter++) {
						signal_handler_connect(
							signal_handler,
							cb_iter->first,
							SourceEventTrigger::Call,
							&cb_iter->second
						);
					}
				}
				m_sources.insert(std::make_pair(source->source(), const_cast<Source*>(source)));
			}
		}

		void
		removeSource(std::map<obs_source_t*, Source*>::iterator source) {
			if(source != m_sources.end()) {
				signal_handler_t* signal_handler = obs_source_get_signal_handler(source->first);
				if(signal_handler != nullptr) {
					for(auto cb_iter = m_callbacks.begin(); cb_iter != m_callbacks.end(); cb_iter++) {
						signal_handler_disconnect(
							signal_handler,
							cb_iter->first,
							SourceEventTrigger::Call,
							&cb_iter->second
						);
					}
				}
				m_sources.erase(source);
			}
		}

		void
		removeSource(const Source* source) {
			removeSource(m_sources.find(source->source()));
		}

		void
		removeAll() {
			for(auto iter = m_sources.begin(); iter != m_sources.end();) {
				auto remove = iter;
				iter++;
				removeSource(remove);
			}
		}

	private:

		void
		registerCallbacks() {
			m_callbacks["mute"] = boost::bind(&SourceEventTrigger::onMute, this, _1, _2);
			m_callbacks["update_flags"] = boost::bind(&SourceEventTrigger::onFlags, this, _1, _2);
			m_callbacks["rename"] = boost::bind(&SourceEventTrigger::onRename, this, _1, _2);
		}

		void
		onMute(obs_source_t* obs_source, calldata_t* data_ptr) {
			auto source = m_sources.find(obs_source);
			if(source != m_sources.end()) {
				obs::source::data data = obs::source::data{
					obs::source::event::MUTE,
					source->second
				};
				if(calldata_get_bool(data_ptr, "muted", &data.data.boolean_value)) {
					m_event.notifyEvent<const obs::source::data&>(data.event, data);
				}
			}
		}

		void
		onFlags(obs_source_t* obs_source, calldata_t* data_ptr) {
			auto source = m_sources.find(obs_source);
			if(source != m_sources.end()) {
				obs::source::data data = obs::source::data{
					obs::source::event::FLAGS,
					source->second
				};
				if(calldata_get_int(data_ptr, "flags", &data.data.uint_value)) {
					m_event.notifyEvent<const obs::source::data&>(data.event, data);
				}
			}
		}

		void
		onRename(obs_source_t* obs_source, calldata_t* data_ptr) {
			auto source = m_sources.find(obs_source);
			if(source != m_sources.end()) {
				obs::source::data data = obs::source::data{
					obs::source::event::RENAMED,
					source->second
				};
				if(calldata_get_string(data_ptr, "new_name", &data.data.string_value)) {
					m_event.notifyEvent<const obs::source::data&>(data.event, data);
				}
			}
		}

};