#pragma once

/*
 * Std Includes
 */
#include <mutex>
#include <condition_variable>
#include <thread>
#include <map>

/*
 * Plugin Includes
 */
#include "include/common/SharedVariables.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class SharedVariablesManager {

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	public:

		static SharedVariablesManager& instance() {
			static SharedVariablesManager _instance;
			return _instance;
		}
	
	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::map<std::string, SharedVariable*> m_variables;

		std::mutex m_mutex;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	private:

		SharedVariablesManager() = default;
		
		SharedVariablesManager(SharedVariablesManager&&) = delete;

		SharedVariablesManager(SharedVariablesManager&) = delete;

		~SharedVariablesManager() {
			
			for(auto i = m_variables.begin(); i != m_variables.end(); i++)
				delete (*i).second;
		}

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		template<typename T>
		SharedVariableT<T>* getVariable(std::string name) {
			static std::string message = "Typecast Exception : Impossible convertion between "
				"shared variables.";

			SharedVariable* var = nullptr;
			auto iter = m_variables.find(name);
			if(iter != m_variables.end()) {
				var = (*iter).second;
				if(dynamic_cast<SharedVariableT<T>*>(var) == nullptr) {
					throw std::exception(message.c_str());
				}
			}
			else {
				std::lock_guard<std::mutex> lock(m_mutex);
				iter = m_variables.find(name);
				if(iter == m_variables.end()) {
					var = new SharedVariableT<T>();
					m_variables[name] = var;
				}
				else {
					if(dynamic_cast<SharedVariableT<T>*>((*iter).second) == nullptr) {
						var = nullptr;
						throw std::exception(message.c_str());
					}
					else
						var = (*iter).second;
				}
			}

			return reinterpret_cast<SharedVariableT<T>*>(var);
		}

	/*
	====================================================================================================
		Operators
	====================================================================================================
	*/
	private:
		
		SharedVariablesManager operator=(const SharedVariablesManager&) = delete;

		SharedVariablesManager& operator=(SharedVariablesManager&&) = delete;

};

/*
========================================================================================================
	Global Functions
========================================================================================================
*/

template<typename T>
SharedVariableT<T>& shared_variable(std::string name) {
	SharedVariableT<T>* var = SharedVariablesManager::instance().getVariable<T>(name);
	if(var == nullptr) {
		throw std::exception("SharedVariable Exception: Try to dereference a nullptr value.");
	}
	return *var;
}

template<typename T>
SharedVariableT<T>& shared_variable(std::string name, const T& value) {
	SharedVariableT<T>* var = SharedVariablesManager::instance().getVariable<T>(name);
	if(var == nullptr) {
		throw std::exception("SharedVariable Exception: Try to dereference a nullptr value.");
	}
	return *var = value;
}