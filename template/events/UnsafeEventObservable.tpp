/*
 * Plugin Includes
 */
#include "include/events/EventObservable.hpp"
#include "include/global.h"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

template<typename T>
UnsafeEventObservable<T>::UnsafeEventObservable() {

}

template<typename T>
UnsafeEventObservable<T>::~UnsafeEventObservable() {
	m_eventHandlers.clear();
}

/*
========================================================================================================
	Event Observers Handling
========================================================================================================
*/

template<typename T>
void
UnsafeEventObservable<T>::addEventHandler(const T& event, const EventObserver<T>* eventHandler) {
	EventObserver<T>::FuncWrapper* callback = eventHandler->callback(event);
	if(callback != nullptr && 
			m_eventHandlers.contains(event) && !m_eventHandlers[event].contains(callback))
		m_eventHandlers[event].insert(callback);
}

template<typename T>
void
UnsafeEventObservable<T>::remEventHandler(const T& event, const EventObserver<T>* eventHandler) {
	EventObserver<T>::FuncWrapper* callback = eventHandler->callback(event);
	if(callback != nullptr && 
			m_eventHandlers.contains(event) && m_eventHandlers[event].contains(callback))
		m_eventHandlers[event].remove(callback);
}

/*
========================================================================================================
	Events Handling
========================================================================================================
*/

template<typename T>
void
UnsafeEventObservable<T>::addEvent(const T& event) {
	if(!m_eventHandlers.contains(event))
		m_eventHandlers[event] = QSet<EventObserver<T>::FuncWrapper*>();
}

template<typename T>
void
UnsafeEventObservable<T>::removeEvent(const T& event) {
	if(m_eventHandlers.contains(event)) {
		m_eventHandlers.remove(event);
	}
}

template<typename T>
bool
UnsafeEventObservable<T>::notifyEvent(const T& event) const {
	bool result = m_eventHandlers.contains(event);
	if(result) {
		for(auto i = m_eventHandlers[event].begin(); i != m_eventHandlers[event].end(); i++)
			result |= (*i)->call();
	}
	return result;
}

template<typename T>
template<typename B>
bool
UnsafeEventObservable<T>::notifyEvent(const T& event, const B& data) const {
	bool result = m_eventHandlers.contains(event);
	if(result) {
		for(auto i = m_eventHandlers[event].begin(); i != m_eventHandlers[event].end(); i++)
			result |= (*i)->call<B>(data);
	}
	return result;
}