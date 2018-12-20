#pragma once

/*
 * STL Includes
 */
#include <fstream>
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
#include "include/obs/Collection.hpp"
//#include "include/obs/Scene.hpp"
//#include "include/obs/Item.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class OBSManager {

	/*
	====================================================================================================
		Types Definitions
	====================================================================================================
	*/
	private:

		class FileLoader {

			/*
			============================================================================================
				Instance Data Members
			============================================================================================
			*/
			private:

				std::fstream m_stream;

			/*
			============================================================================================
				Constructors / Destructor
			============================================================================================
			*/
			public:

				FileLoader(const char* filename, std::ios_base::openmode mode = std::ios::in);

				~FileLoader();

			/*
			============================================================================================
				Instance Methods
			============================================================================================
			*/
			public:

				bool
				open(const char* filename, std::ios_base::openmode mode);

				size_t
				read(char* buffer, size_t size);

				size_t
				write(char* buffer, size_t size);

		};

	/*
	====================================================================================================
		Static Class Attributes
	====================================================================================================
	*/
	private:

		static unsigned long long _last_registered_id;

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::map<unsigned long long, std::shared_ptr<Collection>> m_collections;

		mutable Collection* m_activeCollection;

		bool m_isLoadingCollections;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		OBSManager();

		~OBSManager();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void
		loadCollections();

		void
		saveCollections();

		obs::collection_event
		updateCollections(std::shared_ptr<Collection>& collection_updated);

		void
		loadScenes(Collection& collection);

		obs::scene_event
		updateScenes(Collection& collection, std::shared_ptr<Scene>& scene_updated);

		Collection*
		activeCollection() const;

		bool
		activeCollection(unsigned long long id);

		Collection*
		collection(unsigned long long id) const;

		Collections
		collections() const;

		bool
		isLoadingCollections() const;

	private:

		void
		extractFromOBSCollections(std::map<std::string, std::shared_ptr<Collection>>& collections);

};