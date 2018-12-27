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
#include "include/obs/Source.hpp"

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

		bool active;

		bool switching;

	private:

		OBSStorage<Scene> m_scenes;

		OBSStorage<Source> m_sources;

		mutable Scene* m_activeScene;

		uint16_t m_lastSceneID;

		uint16_t m_lastSourceID;

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

		Scene*
		addScene(obs_source_t* scene);

		std::shared_ptr<Scene>
		removeScene(Scene& scene);

		std::shared_ptr<Scene>
		renameScene(Scene& scene, const char* name);

		Source*
		addSource(obs_source_t* source);

		std::shared_ptr<Source>
		removeSource(Source& source);

		std::shared_ptr<Source>
		renameSource(Source& source, const char* name);

		void
		loadSources();

		void
		loadScenes();

		void
		synchronize();

		obs::scene::event
		updateScenes(std::shared_ptr<Scene>& scene_updated);

		bool
		switchScene(uint16_t id);

		bool
		switchScene(const char* name);

		Scene*
		activeScene() const;

		Scenes
		scenes() const;

		Sources
		sources() const;

		Memory
		toMemory(size_t& size) const;

};