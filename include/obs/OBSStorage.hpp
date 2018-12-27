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
		
		std::map<uint16_t, std::shared_ptr<T>> m_pointers;

		std::map<std::string, uint16_t> m_idx;

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		std::shared_ptr<T>&
		push(T* storable) {
			std::map<uint16_t, std::shared_ptr<T>>::iterator iter = m_pointers.find(storable->id());
			if(iter != m_pointers.end()) {
				return iter->second;
			}
			
			m_idx[storable->name()] = storable->id();
			return m_pointers.emplace(storable->id(), storable).first->second;
		}

		std::shared_ptr<T>&
		push(const std::shared_ptr<T>& ptr) {
			std::map<uint16_t, std::shared_ptr<T>>::iterator iter = m_pointers.find(ptr->id());
			if(iter != m_pointers.end()) {
				return iter->second;
			}

			m_idx[ptr->name()] = ptr->id();
			return m_pointers.emplace(ptr->id(), ptr).first->second;
		}

		std::shared_ptr<T>
		pop(uint16_t identifier) {
			std::map<uint16_t, std::shared_ptr<T>>::iterator iter = m_pointers.find(identifier);
			if(iter != m_pointers.end()) {
				std::shared_ptr<T> ptr = iter->second;
				m_pointers.erase(iter);
				m_idx.erase(ptr->name());
				return ptr;
			}
			return nullptr;
		}

		std::shared_ptr<T>
		pop(const std::string& name) {
			std::map<std::string, uint16_t>::iterator iter = m_idx.find(name);
			if(iter != m_idx.end()) {
				std::shared_ptr<T> ptr = m_pointers[iter->second];
				m_pointers.erase(iter->second);
				m_idx.erase(iter);
				return ptr;
			}
			return nullptr;
		}

		std::shared_ptr<T>
		move(const std::string name, const std::string new_name) {
			std::map<std::string, uint16_t>::iterator iter = m_idx.find(name);
			if(iter != m_idx.end()) {
				if(name.compare(new_name) == 0) {
					auto ptr = m_pointers.find(iter->second)->second;
					return ptr;
				}
				else {
					m_idx.emplace(new_name, iter->second);
					auto ptr = m_pointers.find(iter->second)->second;
					ptr->name(new_name);
					m_idx.erase(iter);
					return ptr;
				}
			}
			return nullptr;
		}

		typename std::map<uint16_t, std::shared_ptr<T>>::const_iterator
		begin() const {
			return m_pointers.begin();
		}

		typename std::map<uint16_t, std::shared_ptr<T>>::const_iterator
		end() const {
			return m_pointers.end();
		}

		size_t
		size() const {
			return m_pointers.size();
		}

	/*
	====================================================================================================
		Operators
	====================================================================================================
	*/
	public:

		T*
		operator[](const std::string& name) const {
			std::map<std::string, uint16_t>::const_iterator iter = m_idx.find(name);
			if(iter != m_idx.end()) {
				return m_pointers.find(iter->second)->second.get();
			}
			return nullptr;
		}

		T*
		operator[](uint16_t identifier) const {
			std::map<uint16_t, std::shared_ptr<T>>::const_iterator iter = m_pointers.find(identifier);
			if(iter != m_pointers.end()) {
				return iter->second.get();
			}
			return nullptr;
		}

};