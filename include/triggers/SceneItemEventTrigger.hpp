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

class SceneItemEventTrigger : public EventTrigger<SceneItemEventTrigger, obs::item::event> {

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	private:

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

		void
		triggerAddedItem(obs_scene_t* scene, obs_sceneitem_t* item) {
			auto iter = m_scenes.find(scene);
			if(iter == m_scenes.end()) return;

			obs::item::data data = { obs::item::event::ADDED, iter->second };
			data.sceneitem = item;

			m_event.notifyEvent<const obs::item::data&>(obs::item::event::ADDED, data);
		}

		void
		triggerRemovedItem(obs_scene_t* scene, obs_sceneitem_t* item) {
			auto scene_ref = m_scenes.find(scene);
			if(scene_ref == m_scenes.end()) return;

			auto item_ref = m_items.find(item);
			if(item_ref == m_items.end()) return;

			obs::item::data data = { obs::item::event::REMOVED, scene_ref->second };
			data.item = const_cast<Item*>(item_ref->second);

			m_event.notifyEvent<const obs::item::data&>(obs::item::event::REMOVED, data);
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

			m_event.notifyEvent<const obs::item::data&>(event, data);
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