#pragma once

/*
 * Std Includes
 */
#include <memory>

/*
 * Qt Includes
 */
#include <map>
#include <vector>

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

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class Collection;

class Source;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

typedef Source* SourcePtr;

typedef struct Sources {
	const Collection* collection;
	std::vector<SourcePtr> sources;
} Sources;

class Source : public OBSStorable {

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

		static Source* buildFromMemory(Collection* collection, Memory& memory);

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::set<Source**> m_references;

		Collection* m_parentCollection;

		obs_source_t* m_source;

		std::string m_type;

		bool m_audio;

		bool m_muted;

		bool m_registrable;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		Source(Collection* collection, uint16_t id, obs_source_t* source, bool registrable = true);

		Source(Collection* source, uint16_t id, std::string name, bool registrable = true);

		virtual ~Source();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void
		addReference(Source** source);

		void
		removeReference(Source** source);

		bool
		registrable() const;

		Memory
		toMemory(size_t& size) const;

		Collection*
		collection() const;

		void
		source(obs_source_t* obs_source);

		obs_source_t*
		source() const;

		const char*
		type() const;

		bool
		muted() const;

		bool
		muted(bool mute_state, bool rpc_action = false);

		bool
		audio() const;

		void
		audio(uint64_t flags);

};