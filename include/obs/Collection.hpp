#pragma once

/*
 * STL Includes
 */
#include <map>
#include <vector>
#include <memory>

/*
 * OBS Includes
 */
#include <obs.h>
#include <obs-frontend-api/obs-frontend-api.h>
#include <obs-module.h>

/*
 * Plugin Includes
 */
#include "include/common/Memory.hpp"
#include "include/obs/OBSStorage.hpp"
#include "include/obs/OBSEvents.hpp"
#include "include/obs/Scene.hpp"

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class Collection;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

typedef Collection* CollectionPtr;

typedef std::vector<Collection*> Collections;

class Collection : public OBSStorable {

	/*
	====================================================================================================
		Constants
	====================================================================================================
	*/
	public:

		static const unsigned int MAX_NAME_LENGTH = 99;

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	public:

		static Collection* buildFromMemory(Memory& memory);

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	public:

		bool switching;

	private:

		OBSStorage<Scene> m_scenes;

		mutable Scene* m_activeScene;

		uint16_t m_lastSceneID;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		Collection(uint16_t id, std::string name);

		~Collection();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void
		makeActive();

		void
		loadScenes();

		void
		synchronize();

		obs::scene_event
		updateScenes(std::shared_ptr<Scene>& scene_updated);

		bool
		switchScene(uint16_t id);

		bool
		switchScene(const char* name);

		Scene*
		activeScene() const;

		Scenes
		scenes() const;

		Memory
		toMemory(size_t& size) const;

};