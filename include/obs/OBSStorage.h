#pragma once

/*
 * STL Includes
 */
#include <map>
#include <set>

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class OBSStorable;

template<typename T>
using is_storable = std::is_base_of<OBSStorable, T>;

template<typename T, bool = is_storable<T>::value>
class OBSStorage;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class OBSStorable {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	protected:

		uint16_t m_identifier;

		std::string m_name;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	protected:

		OBSStorable(uint16_t identifier, const std::string& name) :
			m_identifier(identifier),
			m_name(name) {
		}

	/*
	====================================================================================================
		Accessors
	====================================================================================================
	*/
	public:

		const std::string&
		name() const {
			return m_name;
		}

		std::string&
		name() {
			return m_name;
		}

		void
		name(const std::string& name) {
			m_name = name;
		}

		uint16_t
		id() const {
			return m_identifier;
		}

		uint16_t&
		id() {
			return m_identifier;
		}

		void
		id(uint16_t identifier) {
			m_identifier = identifier;
		}

};

template<typename T>
class OBSStorage<T, false> {

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		OBSStorage() {
			static_assert(false, "T is not OBSStorable");
		}

};

template<typename T>
class OBSStorage<T, true> {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::set<std::shared_ptr<T>> m_pointers;

		std::map<std::string, std::shared_ptr<T>*> m_ptrByName;

		std::map<uint16_t, std::shared_ptr<T>*> m_ptrById;

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		std::shared_ptr<T>
		push(T* storable) {
			for(auto iter = m_pointers.begin(); iter != m_pointers.end(); iter++) {
				if((*iter).get() == storable)
					return (*iter);
			}
			auto iter = m_pointers.insert(std::shared_ptr<T>(storable));
			m_ptrByName[storable->name()] = &(*iter);
			m_ptrById[storable->id()] = &(*iter);

			return *iter;
		}

		std::shared_ptr<T>
		pop(uint16_t identifier) {
			auto iter = m_ptrById.find(identifier);
			if(iter != m_ptrById.end()) {
				std::shared_ptr<T> value = *(iter->second);
				m_ptrByName.erase(value->name());
				m_ptrById.erase(value->id());
				m_pointers.erase(value);
				return value;
			}
			return nullptr;
		}

		std::shared_ptr<T>
		pop(const std::string& name) {
			auto iter = m_ptrByName.find(name);
			if(iter != m_ptrByName.end()) {
				std::shared_ptr<T> value = *(iter->second);
				m_ptrByName.erase(value->name());
				m_ptrById.erase(value->id());
				m_pointers.erase(value);
				return value;
			}
			return nullptr;
		}

	/*
	====================================================================================================
		Operators
	====================================================================================================
	*/
	public:

		T*
		operator [](const std::string& name) {
			auto iter = m_ptrByName.find(name);
			if(iter != m_ptrByName.end()) {
				return *(iter->second).get();
			}
			return nullptr;
		}

		T*
		operator [](uint16_t identifier) {
			auto iter = m_ptrById.find(identifier);
			if(iter != m_ptrById.end()) {
				return *(iter->second).get();
			}
			return nullptr;
		}

};