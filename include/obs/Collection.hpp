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

class Collection {

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

		std::string m_name;

		unsigned long long m_identifier;

		mutable Scene* m_activeScene;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		Collection(unsigned long long id, std::string name);

		~Collection();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void
		extractFromOBSScenes(unsigned long long& next_scene_identifier);

		obs::scene_event
		updateScenes(unsigned long long& next_scene_identifier, std::shared_ptr<Scene>& scene_updated);

		Scene*
		activeScene() const;

		Memory
		toMemory(size_t& size) const;

		unsigned long long
		id() const;

		std::string
		name() const;

		void
		name(std::string new_name);

		Scenes
		scenes() const;

};