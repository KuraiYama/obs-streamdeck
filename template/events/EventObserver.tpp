/*
 * Plugin Includes
 */
#include "include/events/EventObserver.hpp"
#include "include/global.h"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

template<typename E>
EventObserver<E>::~EventObserver() {
	for(auto i = m_eventHandlers.begin(); i != m_eventHandlers.end(); i++)
		delete i.value();

	m_eventHandlers.clear();
}

template<typename T, typename E>
EventObserver<T, E>::EventObserver() :
	EventObserver<E>() {
}

template<typename T, typename E>
EventObserver<T, E>::~EventObserver() {
}

/*
========================================================================================================
	Callback Handling
========================================================================================================
*/

template<typename E>
typename EventObserver<E>::FuncWrapper*
EventObserver<E>::callback(const E& event) const {
	return m_eventHandlers.contains(event) ? m_eventHandlers[event] : nullptr;
}

template<typename T, typename E>
template<typename B>
void
EventObserver<T, E>::registerCallback(
	const E& event, 
	typename FuncWrapperB<B>::Callback handler,
	T* caller
) {
	if(!m_eventHandlers.contains(event)) {
		m_eventHandlers[event] = new FuncWrapperB<B>(handler, caller, this);
	}
}

template<typename T, typename E>
void
EventObserver<T, E>::registerCallback(
	const E& event, 
	typename FuncWrapperB<void>::Callback handler,
	T* caller
) {
	if(!m_eventHandlers.contains(event)) {
		m_eventHandlers[event] = new FuncWrapperB<void>(handler, caller, this);
	}
}

template<typename T, typename E>
void
EventObserver<T, E>::unregisterCallback(const E& event) {
	if(m_eventHandlers.contains(event)) {
		delete m_eventHandlers[event];
		m_eventHandlers.remove(event);
	}
}

/*
========================================================================================================
	Call Helpers
========================================================================================================
*/

template<typename E>
template<typename B>
bool
EventObserver<E>::call(const E& event, const B& data) const {
	QMap<E, FuncWrapper*>::const_iterator handler = m_eventHandlers.find(event);
	if(handler != m_eventHandlers.end()) {
		return (*handler)->call<B>(data);
	}

	return false;
}

template<typename E>
bool
EventObserver<E>::call(const E& event) const {
	QMap<E, FuncWrapper*>::const_iterator handler = m_eventHandlers.find(event);
	if(handler != m_eventHandlers.end()) {
		return (*handler)->call();
	}

	return false;
}