/*
 * Plugin Includes
 */
#include "include/events/EventTrigger.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

template<typename TriggerType, typename EventType>
EventTrigger<TriggerType, EventType>::EventTrigger() :
	EventTriggerTyped<EventType>() {
	static_assert(is_trigger_of<TriggerType, EventType>::value);
}
	
/*
========================================================================================================
	Event Handling
========================================================================================================
*/

template<typename EventType>
void
EventTriggerTyped<EventType>::addHandler(EventHandler<EventType> handler) {
	m_event.addEvent(handler.first);
	m_event.addEventHandler(handler.first, handler.second);
}

template<typename TriggerType, typename EventType>
template<typename ParamType>
void
EventTrigger<TriggerType, EventType>::notify(
	void* trigger,
	const EventType& event,
	const ParamType& param
) {
	TriggerType* trigger_typed = reinterpret_cast<TriggerType*>(trigger);
	trigger_typed->m_event.notifyEvent<ParamType>(event, param);
}

template<typename TriggerType, typename EventType>
void
EventTrigger<TriggerType, EventType>::notify(void* trigger, const EventType& event) {
	TriggerType* trigger_typed = reinterpret_cast<TriggerType*>(trigger);
	trigger_typed->m_event.notifyEvent(event);
}