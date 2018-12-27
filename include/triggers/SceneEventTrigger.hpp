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

class SceneEventTrigger : public EventTrigger<SceneEventTrigger, obs::scene::event> {

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
			if(
				calldata_get_ptr(data, "source", &source) &&
				strcmp(obs_source_get_id(source), "scene") == 0
			) {
				auto trigger_ref = reinterpret_cast<SceneEventTrigger*>(trigger);
				obs::scene::data data = obs::scene::data { obs::scene::event::ADDED, nullptr };
				data.obs_source = source;
				trigger_ref->m_event.notifyEvent<const obs::scene::data&>(data.event, data);
			}
		}

		static void
		OnSourceDestroyed(void* trigger, calldata_t* data) {
			if(!Service::_obs_started) return;

			obs_source_t* source = nullptr;
			if(
				calldata_get_ptr(data, "source", &source) &&
				strcmp(obs_source_get_id(source), "scene") == 0
			) {
				auto trigger_ref = reinterpret_cast<SceneEventTrigger*>(trigger);
				auto scene_it = trigger_ref->m_scenes.find(source);
				if(scene_it != trigger_ref->m_scenes.end()) {
					obs::scene::data data = obs::scene::data{
						obs::scene::event::REMOVED,
						scene_it->second
					};
					trigger_ref->m_event.notifyEvent<const obs::scene::data&>(data.event, data);
				}
			}
		}

		static void
		OnSourceRenamed(void* trigger, calldata_t* data) {
			if(!Service::_obs_started) return;

			obs_source_t* source = nullptr;
			if(
				calldata_get_ptr(data, "source", &source) &&
				strcmp(obs_source_get_id(source), "scene") == 0
			) {
				auto trigger_ref = reinterpret_cast<SceneEventTrigger*>(trigger);
				auto scene_it = trigger_ref->m_scenes.find(source);
				if(scene_it != trigger_ref->m_scenes.end()) {
					obs::scene::data ev_data = obs::scene::data{
						obs::scene::event::RENAMED,
						scene_it->second
					};
					calldata_get_string(data, "new_name", &ev_data.name);
					trigger_ref->m_event.notifyEvent<const obs::scene::data&>(ev_data.event, ev_data);
				}
			}
		}

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::map<obs_source_t*, Scene*> m_scenes;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		SceneEventTrigger() :
			EventTrigger<SceneEventTrigger, obs::scene::event>() {

			signal_handler_t* signal_handler = obs_get_signal_handler();
			if(signal_handler != nullptr) {
				signal_handler_connect(
					signal_handler,
					"source_create",
					SceneEventTrigger::OnSourceCreated,
					this
				);

				signal_handler_connect(
					signal_handler,
					"source_remove",
					SceneEventTrigger::OnSourceDestroyed,
					this
				);

				signal_handler_connect(
					signal_handler,
					"source_destroy",
					SceneEventTrigger::OnSourceDestroyed,
					this
				);

				signal_handler_connect(
					signal_handler,
					"source_rename",
					SceneEventTrigger::OnSourceRenamed,
					this
				);
			}
		}

		~SceneEventTrigger() {

			removeAll();

			signal_handler_t* signal_handler = obs_get_signal_handler();
			if(signal_handler != nullptr) {
				signal_handler_disconnect(
					signal_handler,
					"source_create",
					SceneEventTrigger::OnSourceCreated,
					this
				);

				signal_handler_disconnect(
					signal_handler,
					"source_remove",
					SceneEventTrigger::OnSourceDestroyed,
					this
				);

				signal_handler_disconnect(
					signal_handler,
					"source_destroy",
					SceneEventTrigger::OnSourceDestroyed,
					this
				);

				signal_handler_disconnect(
					signal_handler,
					"source_rename",
					SceneEventTrigger::OnSourceRenamed,
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
		addScene(const Scene* scene) {
			obs_source_t* source = obs_scene_get_source(scene->scene());
			if(m_scenes.find(source) == m_scenes.end()) {
				m_scenes.insert(std::make_pair(source, const_cast<Scene*>(scene)));
			}
		}

		void
		removeScene(const obs_scene_t* scene) {
			obs_source_t* source = obs_scene_get_source(scene);
			if(source != nullptr) {
				auto iter = m_scenes.find(source);
				if(iter != m_scenes.end())
					m_scenes.erase(iter);
			}
		}

		void
		removeAll() {
			m_scenes.clear();
		}

};