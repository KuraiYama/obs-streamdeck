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

class SceneItemEventTrigger :
	public EventTrigger<SceneItemEventTrigger, obs::item::event>
#ifdef USE_SCENE_BY_FRONTEND
	, public EventTrigger<SceneItemEventTrigger, obs::frontend::event>
#endif
	{

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	private:

#ifdef USE_SCENE_BY_FRONTEND
		static void
		OnSceneRename(void* trigger, calldata_t* data) {
			if(!Service::_obs_started) return;

			obs_source_t* source = nullptr;
			const char* name;
			if(
				calldata_get_ptr(data, "source", &source) &&
				calldata_get_string(data, "new_name", &name)
			) {
				obs_scene_t* scene = obs_scene_from_source(source);
	
				SceneItemEventTrigger& trigger_ref = *reinterpret_cast<SceneItemEventTrigger*>(trigger);
				trigger_ref.triggerRenamedScene(scene, name);
			}
		}
#endif

		static void
		OnItemAdded(void* trigger, calldata_t* data) {
			if(!Service::_obs_started) return;

			obs_scene_t* scene = nullptr;
			obs_sceneitem_t* item = nullptr;
			bool result =
				calldata_get_ptr(data, "scene", &scene) &&
				calldata_get_ptr(data, "item", &item);

			if(!result) return;

			SceneItemEventTrigger& trigger_ref = *reinterpret_cast<SceneItemEventTrigger*>(trigger);
			trigger_ref.triggerAddedItem(scene, item);
		}

		static void
		OnItemRemoved(void* trigger, calldata_t* data) {
			if(!Service::_obs_started) return;

			obs_scene_t* scene = nullptr;
			obs_sceneitem_t* item = nullptr;
			bool result =
				calldata_get_ptr(data, "scene", &scene) &&
				calldata_get_ptr(data, "item", &item);

			if(!result) return;

			SceneItemEventTrigger& trigger_ref = *reinterpret_cast<SceneItemEventTrigger*>(trigger);
			trigger_ref.triggerRemovedItem(scene, item);
		}

		static void
		OnItemVisibilityChanged(void* trigger, calldata_t* data) {
			if(!Service::_obs_started) return;

			obs_scene_t* scene = nullptr;
			obs_sceneitem_t* item = nullptr;
			bool visible = false;
			bool result =
				calldata_get_ptr(data, "scene", &scene) &&
				calldata_get_ptr(data, "item", &item) &&
				calldata_get_bool(data, "visible", &visible);

			if(!result) return;

			SceneItemEventTrigger& trigger_ref = *reinterpret_cast<SceneItemEventTrigger*>(trigger);
			trigger_ref.triggerChangedItem(scene, item, visible);
		}

		static void
		OnItemReordered(void* trigger, calldata_t* data) {
			if(!Service::_obs_started) return;

			obs_scene_t* scene = nullptr;
			bool result = calldata_get_ptr(data, "scene", &scene);

			if(!result) return;

			SceneItemEventTrigger& trigger_ref = *reinterpret_cast<SceneItemEventTrigger*>(trigger);
			trigger_ref.triggerReorderedItems(scene);
		}

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:
		std::map<obs_scene_t*, Scene*> m_scenes;

		std::map<obs_sceneitem_t*, Item*> m_items;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		SceneItemEventTrigger() :
			EventTrigger<SceneItemEventTrigger, obs::item::event>() {
		}

		~SceneItemEventTrigger() {
			removeAll();
		}

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

#ifdef USE_SCENE_BY_FRONTEND
		void
		triggerRenamedScene(obs_scene_t* scene, const char* name) {
			auto iter = m_scenes.find(scene);
			if(iter == m_scenes.end()) return;

			if(iter->second->name().compare(name) == 0) return;

			UnsafeEventObservable<obs::frontend::event>& event_ref =
				EventTrigger<SceneItemEventTrigger, obs::frontend::event>::m_event;
			event_ref.notifyEvent(obs::frontend::event::SCENE_LIST_CHANGED);
		}
#endif

		void
		triggerAddedItem(obs_scene_t* scene, obs_sceneitem_t* item) {
			auto iter = m_scenes.find(scene);
			if(iter == m_scenes.end()) return;

			obs::item::data data = { obs::item::event::ADDED, iter->second };
			data.sceneitem = item;

#ifdef USE_SCENE_BY_FRONTEND
			UnsafeEventObservable<obs::item::event>& event_ref =
				EventTrigger<SceneItemEventTrigger, obs::item::event>::m_event;
			event_ref.notifyEvent<const obs::item::data&>(obs::item::event::ADDED, data);
#else
			m_event.notifyEvent<const obs::item::data&>(obs::item::event::ADDED, data);
#endif
		}

		void
		triggerRemovedItem(obs_scene_t* scene, obs_sceneitem_t* item) {
			auto scene_ref = m_scenes.find(scene);
			if(scene_ref == m_scenes.end()) return;

			auto item_ref = m_items.find(item);
			if(item_ref == m_items.end()) return;

			obs::item::data data = { obs::item::event::REMOVED, scene_ref->second };
			data.item = const_cast<Item*>(item_ref->second);

#ifdef USE_SCENE_BY_FRONTEND
			UnsafeEventObservable<obs::item::event>& event_ref =
				EventTrigger<SceneItemEventTrigger, obs::item::event>::m_event;
			event_ref.notifyEvent<const obs::item::data&>(obs::item::event::REMOVED, data);
#else
			m_event.notifyEvent<const obs::item::data&>(obs::item::event::REMOVED, data);
#endif
		}

		void
		triggerChangedItem(obs_scene_t* scene, obs_sceneitem_t* item, bool visible) {
			auto scene_ref = m_scenes.find(scene);
			if(scene_ref == m_scenes.end()) return;

			auto item_ref = m_items.find(item);
			if(item_ref == m_items.end()) return;

			obs::item::event event = visible ? obs::item::event::SHOWN : obs::item::event::HIDDEN;

			obs::item::data data = { event , scene_ref->second };
			data.item = item_ref->second;

#ifdef USE_SCENE_BY_FRONTEND
			UnsafeEventObservable<obs::item::event>& event_ref =
				EventTrigger<SceneItemEventTrigger, obs::item::event>::m_event;
			event_ref.notifyEvent<const obs::item::data&>(event, data);
#else
			m_event.notifyEvent<const obs::item::data&>(event, data);
#endif
		}

		void
		triggerReorderedItems(obs_scene_t* scene) {
			auto scene_ref = m_scenes.find(scene);
			if(scene_ref == m_scenes.end()) return;

			obs::item::data data = { obs::item::event::REORDER , scene_ref->second };

			m_event.notifyEvent<const obs::item::data&>(obs::item::event::REORDER, data);
		}

		void
		addItem(const Item* item) {
			if(m_items.find(item->item()) == m_items.end()) {
				m_items.insert(std::make_pair(item->item(), const_cast<Item*>(item)));
			}
		}

		void
		addScene(const Scene* scene) {
			if(m_scenes.find(scene->scene()) == m_scenes.end()) {
				signal_handler_t* signal_handler = obs_source_get_signal_handler(scene->source());
				if(signal_handler != nullptr) {

#ifdef USE_SCENE_BY_FRONTEND
					signal_handler_connect(
						signal_handler,
						"rename",
						SceneItemEventTrigger::OnSceneRename,
						this
					);
#endif

					signal_handler_connect(
						signal_handler,
						"item_add",
						SceneItemEventTrigger::OnItemAdded,
						this
					);

					signal_handler_connect(
						signal_handler,
						"item_remove",
						SceneItemEventTrigger::OnItemRemoved,
						this
					);

					signal_handler_connect(
						signal_handler,
						"item_visible",
						SceneItemEventTrigger::OnItemVisibilityChanged,
						this
					);

					signal_handler_connect(
						signal_handler,
						"reorder",
						SceneItemEventTrigger::OnItemReordered,
						this
					);

					m_scenes.insert(std::make_pair(scene->scene(), const_cast<Scene*>(scene)));
				}
			}
		}

		void
		removeAll() {
			for(auto iter = m_scenes.begin(); iter != m_scenes.end();) {
				auto remove = iter;
				iter++;
				removeScene(remove);
			}
			m_scenes.clear();
			m_items.clear();
		}

		void
		removeScene(std::map<obs_scene_t*, Scene*>::iterator scene) {
			if(scene != m_scenes.end()) {
				signal_handler_t* signal_handler = obs_source_get_signal_handler(
					scene->second->source()
				);
				if(signal_handler != nullptr) {

#ifdef USE_SCENE_BY_FRONTEND
					signal_handler_disconnect(
						signal_handler,
						"rename",
						SceneItemEventTrigger::OnSceneRename,
						this
					);
#endif

					signal_handler_disconnect(
						signal_handler,
						"item_add",
						SceneItemEventTrigger::OnItemAdded,
						this
					);

					signal_handler_disconnect(
						signal_handler,
						"item_remove",
						SceneItemEventTrigger::OnItemRemoved,
						this
					);

					signal_handler_disconnect(
						signal_handler,
						"item_visible",
						SceneItemEventTrigger::OnItemVisibilityChanged,
						this
					);

					signal_handler_disconnect(
						signal_handler,
						"reorder",
						SceneItemEventTrigger::OnItemReordered,
						this
					);

					m_scenes.erase(scene);
				}
			}
		}

		void
		removeScene(obs_scene_t* scene) {
			removeScene(m_scenes.find(scene));
		}

		void
		removeItem(obs_sceneitem_t* item) {
			auto iter = m_items.find(item);
			if(iter != m_items.end()) m_items.erase(iter);
		}
};