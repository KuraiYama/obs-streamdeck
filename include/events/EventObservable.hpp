#pragma once

/*
 * STL Includes
 */
#include <iostream>

/*
 * Qt Includes
 */
#include <QMap>
#include <QSet>
#include <QVector>

/*
 * OBS Includes
 */
#include <obs.h>
#include <obs-frontend-api/obs-frontend-api.h>

/*
 * Plugin Includes
 */
#include "include/events/EventObserver.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

template<typename T>
class EventObservable<T> {

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	protected:

		EventObservable();
	
		virtual ~EventObservable() = 0;

	/*
	====================================================================================================
		Abstract Pure Virtual Instance Methods
	====================================================================================================
	*/
	public:

		virtual void addEventHandler(const T& event, const EventObserver<T>* eventHandler) = 0;         
		
		virtual void remEventHandler(const T& event, const EventObserver<T>* eventHandler) = 0;

	protected:

		virtual void addEvent(const T& event) = 0;
		
		virtual void removeEvent(const T& event) = 0;
		
		virtual bool notifyEvent(const T& event) const = 0;

};

template<typename T>
class SafeEventObservable : protected EventObservable<T> {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	protected:

		QMap<T, QSet<EventObserver<T>*>> m_eventHandlers;
	
	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		SafeEventObservable();

		virtual ~SafeEventObservable();
	
	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		virtual void addEventHandler(const T& event, const EventObserver<T>* eventHandler) override;

		virtual void remEventHandler(const T& event, const EventObserver<T>* eventHandler) override;

		virtual void addEvent(const T& event) override;

		virtual void removeEvent(const T& event) override;

		virtual bool notifyEvent(const T& event) const override;

		template<typename B> 
		bool notifyEvent(const T& event, const B& data) const;

};

template<typename T>
class UnsafeEventObservable : public EventObservable<T> {

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	protected:

		QMap<T, QSet<typename EventObserver<T>::FuncWrapper*>> m_eventHandlers;
	
	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		UnsafeEventObservable();

		virtual ~UnsafeEventObservable();
	
	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		virtual void addEventHandler(const T& event, const EventObserver<T>* eventHandler) override;

		virtual void remEventHandler(const T& event, const EventObserver<T>* eventHandler) override;

		virtual void addEvent(const T& event) override;

		virtual void removeEvent(const T& event) override;

		virtual bool notifyEvent(const T& event) const override;

		template<typename B>
		bool notifyEvent(const T& event, const B& data) const;

};

/*
========================================================================================================
	Template Definitions
========================================================================================================
*/

#include "template/events/EventObservable.tpp"
#include "template/events/UnsafeEventObservable.tpp"
#include "template/events/SafeEventObservable.tpp"