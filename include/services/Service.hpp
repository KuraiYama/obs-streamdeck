#pragma once

/*
 * STL Includes
 */
#include <type_traits>

/*
 * Qt Includes
 */
#include <QMap>
#include <QRegexp>

/*
 * OBS Includes
 */
#include <obs-frontend-api/obs-frontend-api.h>

/*
 * Plugin Includes
 */
#include "include/events/EventObservable.hpp"
#include "include/obs/OBSEvents.hpp"
#include "include/rpc/RPCEvents.hpp"

#include "include/obs/Collection.hpp"
#include "include/obs/Scene.hpp"
#include "include/obs/Item.hpp"

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class OBSManager;

class StreamdeckManager;

/*
========================================================================================================
	 Types Definitions
========================================================================================================
*/

class Service {

	/*
	====================================================================================================
		Static Class Attributes
	====================================================================================================
	*/
	public:

		static StreamdeckManager* _streamdeck_manager;

		static OBSManager* _obs_manager;

		static bool _obs_started;

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	protected:

		const char* m_localName;

		const char* m_remoteName;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		virtual ~Service() = 0;

	protected:

		Service(const char* local_name, const char* remote_name);

};

template<typename T>
class ServiceImpl : public Service,
	private EventObserver<T, obs::frontend::event>,
	private EventObserver<T, obs::save::event>,
	private EventObserver<T, obs::output::event>,
	private EventObserver<T, obs::item::event>,
	private EventObserver<T, obs::scene::event>,
	private EventObserver<T, obs::source::event> {

	/*
	====================================================================================================
		Types Aliases
	====================================================================================================
	*/
	private:

		using FrontendHandler = EventObserver<T, obs::frontend::event>;

		using SaveHandler = EventObserver<T, obs::save::event>;

		using OutputHandler = EventObserver<T, obs::output::event>;

		using ItemHandler = EventObserver<T, obs::item::event>;

		using SourceHandler = EventObserver<T, obs::source::event>;

		using SceneHandler = EventObserver<T, obs::scene::event>;

		using RPCHandler = EventObserver<T, rpc::event>;

	/*
	====================================================================================================
		Types Definitions
	====================================================================================================
	*/
	private:

		typedef 
		typename FrontendHandler::template FuncWrapperB<void>::Callback
		obs_frontend_callback;

		typedef 
		typename SaveHandler::template FuncWrapperB<const obs::save::data&>::Callback
		obs_save_callback;

		typedef
		typename OutputHandler::template FuncWrapperB<const obs::output::data&>::Callback
		obs_output_callback;

		typedef
		typename ItemHandler::template FuncWrapperB<const obs::item::data&>::Callback
		obs_item_callback;

		typedef
		typename SourceHandler::template FuncWrapperB<const obs::source::data&>::Callback
		obs_source_callback;

		typedef
		typename SceneHandler::template FuncWrapperB<const obs::scene::data&>::Callback
		obs_scene_callback;

		typedef
		typename RPCHandler::template FuncWrapperB<void>::Callback
		rpc_callback_void;

		typedef
		typename RPCHandler::template FuncWrapperB<const rpc::request&>::Callback
		rpc_callback_typed;

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

		ServiceImpl(const char* local_name, const char* remote_name);

		virtual ~ServiceImpl() = 0;

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	protected:

		rpc::response<void>
		response_void(const rpc::request* data, const char* method) const;

		rpc::response<std::string>
		response_string(const rpc::request* data, const char* method) const;

		rpc::response<bool>
		response_bool(const rpc::request* data, const char* method) const;

		rpc::response<rpc::response_error>
		response_error(const rpc::request* data, const char* method) const;

		rpc::response<std::pair<std::string, std::string>>
		response_string2(const rpc::request* data, const char* method) const;

		rpc::response<Collections>
		response_collections(const rpc::request* data, const char* method) const;

		rpc::response<CollectionPtr>
		response_collection(const rpc::request* data, const char* method) const;

		rpc::response<Scenes>
		response_scenes(const rpc::request* data, const char* method) const;

		rpc::response<ScenePtr>
		response_scene(const rpc::request* data, const char* method) const;

		rpc::response<Sources>
		response_sources(const rpc::request* data, const char* method) const;

		void
		setupEvent(obs::frontend::event event, obs_frontend_callback handler);

		void
		setupEvent(obs::save::event event, obs_save_callback handler);

		void
		setupEvent(obs::output::event event, obs_output_callback handler);

		void
		setupEvent(obs::item::event event, obs_item_callback handler);

		void
		setupEvent(obs::source::event event, obs_source_callback handler);

		void
		setupEvent(obs::scene::event event, obs_scene_callback handler);

		void
		setupEvent(rpc::event event, rpc_callback_void handler);

		void
		setupEvent(rpc::event event, rpc_callback_typed handler);

		bool
		checkResource(const rpc::request* data, const QRegExp& regex) const;

		void
		logInfo(const std::string& message) const;

		void
		logError(const std::string& message) const;

		void
		logWarning(const std::string& message) const;

		inline const char*
		name() const;

		inline StreamdeckManager*
		streamdeckManager() const;

		inline OBSManager*
		obsManager() const;

};

/*
========================================================================================================
	Template Definitions
========================================================================================================
*/

#include "template/services/Service.tpp"