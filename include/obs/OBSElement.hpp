#pragma once

/*
 * OBS Includes
 */
#include <obs.h>

/*
 * Plugin Includes
 */
#include "include/obs/OBSStorable.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class OBSElement : public OBSStorable {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	protected:

		std::string m_name;

		uint16_t m_identifier;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		OBSElement(uint16_t identifier, const std::string& name) :
			m_identifier(identifier),
			m_name(name) {
		}

		virtual ~OBSElement() {
		}

	/*
	====================================================================================================
		Accessors
	====================================================================================================
	*/
	public:

		virtual const std::string&
		name() const override {
			return m_name;
		}

		virtual void
		name(const std::string& name) override {
			m_name = name;
		}

		virtual uint16_t
		id() const override {
			return m_identifier;
		}

		virtual void
		id(uint16_t identifier) override {
			m_identifier = identifier;
		}

};