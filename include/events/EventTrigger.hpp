#pragma once

/*
 * Std Includes
 */
#include <type_traits>

/*
 * Plugin Includes
 */
#include "include/events/EventObservable.hpp"
#include "include/events/EventObserver.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

template<typename EventType>
using EventHandler = std::pair<EventType, EventObserver<EventType>*>;

template<typename EventType>
class EventTriggerTyped {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	protected:

		UnsafeEventObservable<EventType> m_event;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		virtual ~EventTriggerTyped() = default;

		virtual void
		addHandler(EventHandler<EventType> handler);

};

template<typename TriggerType, typename EventType>
class EventTrigger : public EventTriggerTyped<EventType> {

	template<typename TriggerType, typename EventType>
	using is_trigger_of = std::is_base_of<EventTrigger<TriggerType, EventType>, TriggerType>;

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	protected:

		template<typename ParamType>
		static void
		notify(void* trigger, const EventType& event, const ParamType& param);

		static void
		notify(void* trigger, const EventType& event);

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	protected:

		EventTrigger();

};

/*
 * Template Definitions
 */
#include "template/events/EventTrigger.tpp"