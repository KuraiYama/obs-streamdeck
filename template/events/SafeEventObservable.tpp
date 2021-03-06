/*
 * Plugin Includes
 */
#include "include/events/EventObservable.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

template<typename T>
SafeEventObservable<T>::SafeEventObservable() {
}

template<typename T>
SafeEventObservable<T>::~SafeEventObservable() {
	m_eventHandlers.clear();
}

/*
========================================================================================================
	Event Observers Handling
========================================================================================================
*/

template<typename T>
void
SafeEventObservable<T>::addEventHandler(const T& event, const EventObserver<T>* event_handler) {
	if(m_eventHandlers.contains(event) && 
			!m_eventHandlers[event].contains((EventObserver<T>*)event_handler))
		m_eventHandlers[event].insert((EventObserver<T>*)event_handler);
}

template<typename T>
void
SafeEventObservable<T>::remEventHandler(const T& event, const EventObserver<T>* event_handler) {
	if(m_eventHandlers.contains(event) && 
			m_eventHandlers[event].contains((EventObserver<T>*)event_handler))
		m_eventHandlers[event].remove((EventObserver<T>*)event_handler);
}

/*
========================================================================================================
	Events Handling
========================================================================================================
*/

template<typename T>
void
SafeEventObservable<T>::addEvent(const T& event) {
	if(!m_eventHandlers.contains(event))
		m_eventHandlers[event] = QSet<EventObserver<T>*>();
}

template<typename T>
void
SafeEventObservable<T>::removeEvent(const T& event) {
	if(m_eventHandlers.contains(event)) {
		m_eventHandlers.remove(event);
	}
}

template<typename T>
bool
SafeEventObservable<T>::notifyEvent(const T& event) const {
	bool result = m_eventHandlers.contains(event);
	if(result) {
		result = m_eventHandlers[event].count() == 0;
		for(auto i = m_eventHandlers[event].begin(); i != m_eventHandlers[event].end(); i++)
			result |= (*i)->call(event);
	}
	return result;
}

template<typename T>
template<typename B>
bool
SafeEventObservable<T>::notifyEvent(const T& event, const B& data) const {
	bool result = m_eventHandlers.contains(event);
	if(result) {
		result = m_eventHandlers[event].count() == 0;
		for(auto i = m_eventHandlers[event].begin(); i != m_eventHandlers[event].end(); i++)
			result |= (*i)->call<B>(event, data);
	}
	return result;
}