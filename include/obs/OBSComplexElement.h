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

class OBSComplexElement : public OBSStorable {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	protected:

		OBSStorable* m_internalStorable;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	protected:

		OBSComplexElement(OBSStorable& internalStorable) :
			m_internalStorable(&internalStorable) {
		}

		virtual ~OBSComplexElement() {
			delete m_internalStorable;
		}

	/*
	====================================================================================================
		Accessors
	====================================================================================================
	*/
	public:

		virtual const std::string&
		name() const override {
			return m_internalStorable->name();
		}

		virtual void
		name(const std::string& name) override {
			m_internalStorable->name(name);
		}

		virtual uint16_t
		id() const override {
			return m_internalStorable->id();
		}

		virtual void
		id(uint16_t identifier) override {
			m_internalStorable->id(identifier);
		}

};