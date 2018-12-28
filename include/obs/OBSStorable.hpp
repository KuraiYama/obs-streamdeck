#pragma once

/*
 * Std Includes
 */
#include <memory>
#include <string>

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class OBSStorable {

	friend class OBSElement;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		OBSStorable() = default;

		virtual ~OBSStorable() = default;

	/*
	====================================================================================================
		Accessors
	====================================================================================================
	*/
	public:

		virtual const std::string&
		name() const = 0;

		virtual void
		name(const std::string& name) = 0;

		virtual uint16_t
		id() const = 0;

		virtual void
		id(uint16_t identifier) = 0;

};

template<typename T>
using is_storable = std::is_base_of<OBSStorable, T>;