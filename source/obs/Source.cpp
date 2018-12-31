/*
 * Plugin Includes
 */
#include "include/obs/Source.hpp"
#include "include/obs/Collection.hpp"
#include "include/common/Logger.hpp"

/*
 * Qt Includes
 */
#include <QString>

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

Source::Source(Collection* collection, uint16_t id, obs_source_t* source, bool registrable) :
	OBSStorable(id, obs_source_get_name(source)),
	m_parentCollection(collection) {
	this->source(source);
	m_registrable = registrable;
}

Source::Source(Collection* collection, uint16_t id, std::string name, bool registrable) :
	OBSStorable(id, name),
	m_parentCollection(collection) {
	m_source = nullptr;
	m_registrable = registrable;
}

Source::~Source() {
	for(auto iter = m_references.begin(); iter != m_references.end(); iter++)
		**iter = nullptr;
}

/*
========================================================================================================
	Builders
========================================================================================================
*/

Source*
Source::buildFromMemory(Collection* collection, Memory& memory) {
	uint16_t id = 0;
	unsigned int namelen = 0;
	char source_name[MAX_NAME_LENGTH];

	if(memory == nullptr)
		return nullptr;

	memory.read((byte*)&id, sizeof(uint16_t));
	memory.read((byte*)&namelen, sizeof(unsigned int));
	source_name[namelen] = 0;
	memory.read(source_name, namelen);

	Source* source = new Source(collection, id, source_name);

	return source;
}


/*
========================================================================================================
	Serialization
========================================================================================================
*/

Memory
Source::toMemory(size_t& size) const {
	/*BLOCK
		id (short)
		namelen (unsigned int)
		name (namelen)
	*/
	unsigned int namelen = static_cast<unsigned int>(strlen(m_name.c_str()));
	size_t block_size = sizeof(uint16_t) + sizeof(unsigned int) + namelen;

	// TODO

	Memory block(block_size);
	block.write((byte*)&m_identifier, sizeof(uint16_t));
	block.write((byte*)&namelen, sizeof(unsigned int));
	block.write((byte*)m_name.c_str(), namelen);

	size += block_size;
	return block;
}

/*
========================================================================================================
	References Helpers
========================================================================================================
*/

void
Source::addReference(Source** source) {
	*source = this;
	m_references.insert(source);
}

void
Source::removeReference(Source** source) {
	if(m_references.find(source) != m_references.end()) {
		*source = nullptr;
		m_references.erase(source);
	}
}

/*
========================================================================================================
	Accessors
========================================================================================================
*/

Collection*
Source::collection() const {
	return m_parentCollection;
}

bool
Source::registrable() const {
	return m_registrable;
}

void
Source::source(obs_source_t* obs_source) {
	m_source = obs_source;
	m_type = obs_source_get_id(m_source);
	uint32_t output_flags = obs_source_get_output_flags(m_source);
	m_audio = (output_flags & OBS_SOURCE_AUDIO) != 0;
	m_muted = obs_source_muted(m_source);
}

obs_source_t*
Source::source() const {
	return m_source;
}

const char*
Source::type() const {
	return m_type.c_str();
}

bool
Source::muted() const {
	return m_parentCollection->active && m_muted;
}

bool
Source::muted(bool mute_state, bool rpc_action) {
	if(m_parentCollection->active && m_audio && m_source != nullptr) {
		m_muted = mute_state;
		if(rpc_action) {
			obs_source_set_muted(m_source, mute_state);
			m_muted = obs_source_muted(m_source);
			return m_muted == mute_state;
		}
		return true;
	}
	return false;
}

bool
Source::audio() const {
	return m_audio;
}

void
Source::audio(uint64_t flags) {
	if(m_parentCollection->active)
		m_audio = (flags & OBS_SOURCE_AUDIO) != 0;
}