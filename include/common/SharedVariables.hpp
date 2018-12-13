#pragma once

/*
 * Std Includes
 */
#include <mutex>
#include <condition_variable>
#include <thread>

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class SharedVariablesManager;

template<typename...> class SharedVariableT;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class SharedVariable {

	friend class SharedVariablesManager;

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	protected:

		std::mutex m_mutex;

		std::condition_variable m_condition;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	protected:

		SharedVariable() : m_mutex() {};

		SharedVariable(const SharedVariable&) = delete;

		SharedVariable(SharedVariable&&) = delete;

		virtual ~SharedVariable() = 0 { }

};

template<typename T>
class SharedVariableT<T> : public SharedVariable {

	/*
	====================================================================================================
		Types Definitions
	====================================================================================================
	*/
	protected:

		typedef bool(*Predicate)(const T& value);

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		T m_internalVariable;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		SharedVariableT() : SharedVariable() {};

		SharedVariableT(T& value) : SharedVariable(), m_internalVariable(value) {};

		SharedVariableT(const SharedVariableT<T>&) = delete;

		SharedVariableT(SharedVariableT<T>&&) = delete;

		virtual ~SharedVariableT() { }

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void wait(Predicate p) {
			std::unique_lock<std::mutex> lock(m_mutex);
			m_condition.wait(lock, [p, this]() { return p(this->m_internalVariable); });
		}

		void set(T value) {
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_internalVariable = value;
			}
			m_condition.notify_all();
		}

		T get() const {
			return m_internalVariable;
		}

	/*
	====================================================================================================
		Operators
	====================================================================================================
	*/
	public:

		SharedVariableT<T>& operator=(const SharedVariableT<T>&) = delete;

		SharedVariableT<T>& operator=(SharedVariableT<T>&&) = delete;

		SharedVariableT<T>& operator=(const T& value) {
			this->set(value);
			return *this;
		}

		operator T() const {
			return m_internalVariable;
		}
};

typedef SharedVariableT<bool>& bool_s;

typedef SharedVariableT<int>& int_s;

typedef SharedVariableT<float>& float_s;

typedef SharedVariableT<double>& double_s;

typedef SharedVariableT<long>& long_s;

typedef SharedVariableT<short>& short_s;

typedef SharedVariableT<char>& char_s;

typedef SharedVariableT<unsigned int>& uint_s;

typedef SharedVariableT<unsigned long>& ulong_s;

typedef SharedVariableT<unsigned short>& ushort_s;

typedef SharedVariableT<unsigned char>& uchar_s;