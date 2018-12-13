#pragma once

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
#include <iostream>

/*
 * Boost Includes
 */
#include <boost/function.hpp>
#include <boost/bind.hpp>

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

template<typename...> class EventObserver;

template<typename...> class EventObservable;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

template<typename E>
class EventObserver<E> {

	/*
	====================================================================================================
		Types Definitions
	====================================================================================================
	*/
	public:

		class FuncWrapper {

			friend class EventObserver<E>;

			/*
			============================================================================================
				Instance Data Members
			============================================================================================
			*/
			private:

				EventObserver<E>* m_handler;
		
			/*
			============================================================================================
				Constructors / Destructor
			============================================================================================
			*/
			protected:

				FuncWrapper(EventObserver<E>* handler) : m_handler(handler) {
				};

				virtual ~FuncWrapper() {
				};

			/*
			============================================================================================
				Abstract Pure Virtual Instance Methods
			============================================================================================
			*/
			protected:
				
				virtual void* getClassAddress() const = 0;

			/*
			============================================================================================
				Instance Methods
			============================================================================================
			*/
			public:
				
				template<typename B>
				bool call(const B& data) const {
					if(this->getClassAddress() == FuncWrapperA<B>::classAddress()) {
						FuncWrapperA<B>* ptr = (FuncWrapperA<B>*)const_cast<FuncWrapper*>(this);
						return FuncWrapperA<B>::invoke(const_cast<const FuncWrapperA<B>*>(ptr), data);
					}
					return false;
				}

				bool call() const {
					if(this->getClassAddress() == FuncWrapperA<void>::classAddress()) {
						FuncWrapperA<void>* ptr = (FuncWrapperA<void>*)const_cast<FuncWrapper*>(this);
						return FuncWrapperA<void>::invoke(const_cast<const FuncWrapperA<void>*>(ptr));
					}
					return false;
				}
			
		};

	protected:

		template<typename A>
		class FuncWrapperA : public FuncWrapper {

			friend class FuncWrapper;

			/*
			============================================================================================
				Static Class Methods
			============================================================================================
			*/
			protected:

				static void _dummy() {
				};

				static bool invoke(const FuncWrapperA<A>* inst, const A& data) {
					return (*inst)(data);
				}
			
			private:

				static void* classAddress() {
					return FuncWrapperA<A>::_dummy;
				}

			/*
			============================================================================================
				Constructors / Destructor
			============================================================================================
			*/
			protected:

				FuncWrapperA(EventObserver<E>* handler) : FuncWrapper(handler) {
				};

				virtual ~FuncWrapperA() {
				};

			/*
			============================================================================================
				Abstract Virtual Pure Operators
			============================================================================================
			*/
			protected:

				virtual bool operator()(const A& data) const = 0;

			/*
			============================================================================================
				Instance Methods
			============================================================================================
			*/
			private:

				void* getClassAddress() const final {
					return FuncWrapperA<A>::_dummy;
				}

		};

		template<>
		class FuncWrapperA<void> : public FuncWrapper {
			
			friend class FuncWrapper;

			/*
			============================================================================================
				Static Class Methods
			============================================================================================
			*/
			protected:

				static void _dummy() {
				};

				static bool invoke(const FuncWrapperA<void>* inst) {
					return (*inst)();
				}

				static void* classAddress() {
					return FuncWrapperA<void>::_dummy;
				}

			/*
			============================================================================================
				Constructors / Destructor
			============================================================================================
			*/
			protected:

				FuncWrapperA(EventObserver<E>* handler) : FuncWrapper(handler) {
				};

				virtual ~FuncWrapperA() {
				};

			/*
			============================================================================================
				Abstract Virtual Pure Operators
			============================================================================================
			*/
			protected:
				
				virtual bool operator()() const = 0;

			/*
			============================================================================================
				Instance Methods
			============================================================================================
			*/
			protected:

				void* getClassAddress() const final {
					return FuncWrapperA<void>::_dummy;
				}

		};

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	protected:

		QMap<E, FuncWrapper*> m_eventHandlers;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		virtual ~EventObserver() = 0;

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		template<typename B>
		bool call(const E& event, const B& data) const;

		bool call(const E& event) const;

		FuncWrapper* callback(const E& event) const;

};

template<class T, typename E>
class EventObserver<T,E> : public EventObserver<E> {

	friend T;

	/*
	====================================================================================================
		Types Definitions
	====================================================================================================
	*/
	public:

		template<typename B>
		class FuncWrapperB : public EventObserver<T,E>::template FuncWrapperA<B> {
			
			friend T;

			friend class EventObserver<T, E>;

			/*
			============================================================================================
				Types Definitions
			============================================================================================
			*/
			public:

				typedef bool (T::*Callback)(B);

			/*
			============================================================================================
				Instance Data Members
			============================================================================================
			*/
			protected:

				boost::function<bool(B)> m_internalFunc;

			/*
			============================================================================================
				Constructors / Destructor
			============================================================================================
			*/
			protected:

				FuncWrapperB(Callback callback, T* caller, EventObserver<E>* handler = caller) :
					FuncWrapperA<B>(handler),
					m_internalFunc(boost::bind(callback, caller, _1)) {
				}

				virtual ~FuncWrapperB() {
				};

			/*
			============================================================================================
				Operators
			============================================================================================
			*/
			protected:

				bool operator()(const B& data) const final {
					return m_internalFunc(data);
				}

		};

		template<>
		class FuncWrapperB<void> : public EventObserver<T, E>::template FuncWrapperA<void> {

			friend T;

			friend class EventObserver<T,E>;

			/*
			============================================================================================
				Types Definitions
			============================================================================================
			*/
			public:

				typedef bool (T::*Callback)(void);

			/*
			============================================================================================
				Instance Data Members
			============================================================================================
			*/
			protected:

				boost::function<bool(void)> m_internalFunc;

			/*
			============================================================================================
				Constructors / Destructor
			============================================================================================
			*/
			protected:

				FuncWrapperB(Callback callback, T* caller, EventObserver<E>* handler = caller) :
					FuncWrapperA<void>(handler),
					m_internalFunc(boost::bind(callback, caller)) {
				}

				virtual ~FuncWrapperB() {
				};

			/*
			============================================================================================
				Operators
			============================================================================================
			*/
			protected:

				bool operator()() const final {
					return m_internalFunc();
				}

		};

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		EventObserver();

		virtual ~EventObserver();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		template<typename B>
		void registerCallback(const E& event, typename FuncWrapperB<B>::Callback handler, T* caller);

		void registerCallback(const E& event, typename FuncWrapperB<void>::Callback, T* caller);

		void unregisterCallback(const E& event);

};

/*
========================================================================================================
	Template Definitions
========================================================================================================
*/

#include "template/events/EventObserver.tpp"