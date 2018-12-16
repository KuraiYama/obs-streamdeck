#pragma once

/*
 * STL Includes
 */
#include <type_traits>

/*
 * Qt Includes
 */
#include <QMap>

/*
 * OBS Includes
 */
#include <obs-frontend-api/obs-frontend-api.h>

/*
 * Plugin Includes
 */
#include "include/events/EventObservable.hpp"
#include "include/streamdeck/StreamDeckManager.hpp"

/*
========================================================================================================
	 Types Definitions
========================================================================================================
*/

enum class obs_save_event { OBS_SAVE_EVENT_SAVED, OBS_SAVE_EVENT_LOADED };

class Service {

	/*
	====================================================================================================
		Static Class Methods
	====================================================================================================
	*/
	private:
	
		static void
		OnObsFrontendEvent(obs_frontend_event event, void* service);

		static void
		OnObsSaveEvent(obs_data_t* save_data, bool saving, void* service);

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	protected:

		const char* m_name;

		StreamdeckManager* m_streamdeckManager;

	protected:

		UnsafeEventObservable<obs_frontend_event> m_frontendEvent;

		UnsafeEventObservable<obs_save_event> m_saveEvent;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		virtual ~Service();

	protected:

		Service(const char* name, StreamdeckManager* streamdeckManager = nullptr);

	/*
	====================================================================================================
		Instance methods
	====================================================================================================
	*/
	protected:

		virtual void
		logger(const std::string& message) const;

};

template<typename T>
class ServiceT : private Service, 
		private EventObserver<T, obs_frontend_event>, 
		private EventObserver<T, obs_save_event>  {

	/*
	====================================================================================================
		Types Aliases
	====================================================================================================
	*/
	private:

		using FrontendHandler = EventObserver<T, obs_frontend_event>;

		using SaveHandler = EventObserver<T, obs_save_event>;

		using RPCHandler = EventObserver<T, Streamdeck::rpc_event>;

	/*
	====================================================================================================
		Types Definitions
	====================================================================================================
	*/
	private:

		typedef 
			typename FrontendHandler::template FuncWrapperB<void>::Callback obs_frontend_callback;

		typedef 
			typename SaveHandler::template FuncWrapperB<const obs_data_t*>::Callback obs_save_callback;

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	protected:

		RPCHandler m_rpcEvent;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	protected:

		ServiceT(const char* name, StreamdeckManager* streamdeckManager = nullptr);

		virtual ~ServiceT() = 0;

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	protected:

		rpc_adv_response<void>
		response_void(const rpc_event_data* data, const char* method) const;

		rpc_adv_response<std::string>
		response_string(const rpc_event_data* data, const char* method) const;

		rpc_adv_response<bool>
		response_bool(const rpc_event_data* data, const char* method) const;

		rpc_adv_response<std::tuple<std::string, std::string>>
		response_string2(const rpc_event_data* data, const char* method) const;

		rpc_adv_response<Collections>
		response_collections(const rpc_event_data* data, const char* method) const;

		rpc_adv_response<Collection*>
		response_collection(const rpc_event_data* data, const char* method) const;

		rpc_adv_response<std::tuple<Collection*, Scenes>>
		response_scenes(const rpc_event_data* data, const char* method) const;

		void
		setupEvent(obs_frontend_event event, obs_frontend_callback handler);

		void
		setupEvent(obs_save_event event, obs_save_callback handler);

		void
		setupEvent(
			Streamdeck::rpc_event event, 
			typename RPCHandler::template FuncWrapperB<void>::Callback handler
		);

		template<typename B>
		void
		setupEvent(
			Streamdeck::rpc_event event, 
			typename RPCHandler::template FuncWrapperB<B>::Callback handler
		);

		void
		logger(const std::string& message) const override;

		const char*
		name() const;

		StreamdeckManager*
		streamdeckManager() const;

	/*
	====================================================================================================
		Operators
	====================================================================================================
	*/
	public:

		operator Service*() const;

};

/*
========================================================================================================
	Template Definitions
========================================================================================================
*/

#include "template/services/Service.tpp"