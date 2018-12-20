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
#include "include/obs/OBSStorage.h"
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
	private:

		std::map<unsigned long long, std::shared_ptr<Scene>> m_scenes;

		mutable Scene* m_activeScene;

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
		extractFromOBSScenes(unsigned long long& next_scene_identifier);

		void
		resourceScenes() const;

		obs::scene_event
		updateScenes(unsigned long long& next_scene_identifier, std::shared_ptr<Scene>& scene_updated);

		Scene*
		activeScene(bool force_reset = false) const;

		bool
		switchScene(unsigned long long id);

		Memory
		toMemory(size_t& size) const;

	/*
	====================================================================================================
		Accessors
	====================================================================================================
	*/
	public:

		Scenes
		scenes() const;

};