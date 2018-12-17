#pragma once

/*
 * Qt Includes
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
//#include "include/obs/Scene.hpp"

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
		Static Class Functions
	====================================================================================================
	*/
	public:

		static bool buildFromBuffer(Collection** collection, char* buffer, size_t size);

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		//std::map<std::string, Scene> m_scenes;

		std::string m_name;

		long long m_identifier;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		Collection(long long id = -1, std::string name = "");

		~Collection();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		size_t
		toBytes(char** buffer) const;

		long long
		id() const;

		std::string
		name() const;

		void
		name(std::string new_name);

		/*Scenes
		scenes() const;*/

};