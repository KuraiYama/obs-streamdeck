/*
 * Plugin Includes
 */
#include "include/services/Service.hpp"
#include "include/Global.h"
#include "include/common/Logger.hpp"
#include "include/obs/OBSManager.hpp"
#include "include/streamdeck/StreamDeckManager.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

template<typename T>
ServiceImpl<T>::ServiceImpl(const char* local_name, const char* remote_name) : 
	Service(local_name, remote_name) {
	static_assert(std::is_base_of<ServiceImpl<T>, T>::value);
}

template<typename T>
ServiceImpl<T>::~ServiceImpl() {
}

/*
========================================================================================================
	Events Handling
========================================================================================================
*/

template<typename T>
void
ServiceImpl<T>::setupEvent(obs::frontend::event event, obs_frontend_callback handler) {
	this->EventObserver<T, obs::frontend::event>::registerCallback(
		event,
		(obs_frontend_callback)handler, 
		reinterpret_cast<T*>(this)
	);
	_obs_manager->addEventHandler(event, this);
}

template<typename T>
void
ServiceImpl<T>::setupEvent(obs::save::event event, obs_save_callback handler) {
	this->EventObserver<T, obs::save::event>::registerCallback<const obs::save::data&>(
		event,
		(obs_save_callback)handler,
		reinterpret_cast<T*>(this)
	);
	_obs_manager->addEventHandler(event, this);
}

template<typename T>
void
ServiceImpl<T>::setupEvent(obs::output::event event, obs_output_callback handler) {
	this->EventObserver<T, obs::output::event>::registerCallback<const obs::output::data&>(
		event,
		(obs_output_callback)handler,
		reinterpret_cast<T*>(this)
	);
	_obs_manager->addEventHandler(event, this);
}

template<typename T>
void
ServiceImpl<T>::setupEvent(obs::item::event event, obs_item_callback handler) {
	this->EventObserver<T, obs::item::event>::registerCallback<const obs::item::data&>(
		event,
		(obs_item_callback)handler,
		reinterpret_cast<T*>(this)
	);
	_obs_manager->addEventHandler(event, this);
}

template<typename T>
void
ServiceImpl<T>::setupEvent(obs::source::event event, obs_source_callback handler) {
	this->EventObserver<T, obs::source::event>::registerCallback<const obs::source::data&>(
		event,
		(obs_source_callback)handler,
		reinterpret_cast<T*>(this)
		);
	_obs_manager->addEventHandler(event, this);
}

template<typename T>
void
ServiceImpl<T>::setupEvent(obs::scene::event event, obs_scene_callback handler) {
	this->EventObserver<T, obs::scene::event>::registerCallback<const obs::scene::data&>(
		event,
		(obs_scene_callback)handler,
		reinterpret_cast<T*>(this)
		);
	_obs_manager->addEventHandler(event, this);
}

template<typename T>
void
ServiceImpl<T>::setupEvent(rpc::event, rpc_callback_void handler) {
	if(_streamdeck_manager == nullptr)
		return;

	_rpcEvent.registerCallback(
		event, 
		(RPCHandler::FuncWrapperB<void>::Callback)handler, reinterpret_cast<T*>(this)
	);
	_streamdeck_manager->addEventHandler(event, &m_rpcEvent);
}

template<typename T>
void
ServiceImpl<T>::setupEvent(rpc::event event, rpc_callback_typed handler) {
	if(_streamdeck_manager == nullptr)
		return;

	m_rpcEvent.registerCallback<const rpc::request&>(
		event, 
		(RPCHandler::FuncWrapperB<const rpc::request&>::Callback)handler, reinterpret_cast<T*>(this)
	);
	_streamdeck_manager->addEventHandler(event, &m_rpcEvent);
}

/*
========================================================================================================
	Logging
========================================================================================================
*/

template<typename T>
void
ServiceImpl<T>::logInfo(const std::string& message) const {
	log_info << QString("[%1] %2")
		.arg(QString(m_localName))
		.arg(QString::fromStdString(message))
		.toStdString() << log_end;
}

template<typename T>
void
ServiceImpl<T>::logError(const std::string& message) const {
	log_error << QString("[%1] %2")
		.arg(QString(m_localName))
		.arg(QString::fromStdString(message))
		.toStdString() << log_end;
}

template<typename T>
void
ServiceImpl<T>::logWarning(const std::string& message) const {
	log_warn << QString("[%1] %2")
		.arg(QString(m_localName))
		.arg(QString::fromStdString(message))
		.toStdString() << log_end;
}

/*
========================================================================================================
	RPC Helpers
========================================================================================================
*/

template<typename T>
bool
ServiceImpl<T>::checkResource(const rpc::request* data, const QRegExp& method) const {
	return data == nullptr ||
		(data->serviceName.compare(m_remoteName) == 0 && method.exactMatch(data->method.c_str()));
}

/*
========================================================================================================
	RPC Response Initialiaztion
========================================================================================================
*/

template<typename T>
rpc::response<void>
ServiceImpl<T>::response_void(const rpc::request* data, const char* method) const {
	return
		rpc::response<void>{
			{data, rpc::event::NO_EVENT, name(), method}
		};
}

template<typename T>
rpc::response<std::string>
ServiceImpl<T>::response_string(const rpc::request* data, const char* method) const {
	return 
		rpc::response<std::string>{ 
			{data, rpc::event::NO_EVENT, name(), method},
			std::string("") 
		};
}

template<typename T>
rpc::response<bool>
ServiceImpl<T>::response_bool(const rpc::request* data, const char* method) const {
	return
		rpc::response<bool>{
			{data, rpc::event::NO_EVENT, name(), method},
			true
		};
}

template<typename T>
rpc::response<std::pair<std::string,std::string>>
ServiceImpl<T>::response_string2(const rpc::request* data, const char* method) const {
	return
		rpc::response<std::pair<std::string,std::string>>{
			{data, rpc::event::NO_EVENT, name(), method},
			std::pair<std::string,std::string>("","")
		};
}

template<typename T>
rpc::response<CollectionPtr>
ServiceImpl<T>::response_collection(const rpc::request* data, const char* method) const {
	return
		rpc::response<Collection*>{
			{data, rpc::event::NO_EVENT, name(), method},
			nullptr
	};
}

template<typename T>
rpc::response<Collections>
ServiceImpl<T>::response_collections(const rpc::request* data, const char* method) const {
	return
		rpc::response<Collections>{
			{data, rpc::event::NO_EVENT, name(), method},
			Collections()
		};
}

template<typename T>
rpc::response<ScenePtr>
ServiceImpl<T>::response_scene(const rpc::request* data, const char* method) const {
	return
		rpc::response<Scene*>{
			{data, rpc::event::NO_EVENT, name(), method},
			nullptr
	};
}

template<typename T>
rpc::response<Scenes>
ServiceImpl<T>::response_scenes(const rpc::request* data, const char* method) const {
	return
		rpc::response<Scenes>{
			{data, rpc::event::NO_EVENT, name(), method},
			{ nullptr, std::vector<ScenePtr>() }
	};
}

template<typename T>
rpc::response<Sources>
ServiceImpl<T>::response_sources(const rpc::request* data, const char* method) const {
	return
		rpc::response<Sources>{
			{data, rpc::event::NO_EVENT, name(), method},
			{ nullptr, std::vector<SourcePtr>() }
	};
}

/*
========================================================================================================
	Accessors
========================================================================================================
*/

template<typename T>
const char*
ServiceImpl<T>::name() const {
	return m_localName;
}

template<typename T>
StreamdeckManager*
ServiceImpl<T>::streamdeckManager() const {
	return _streamdeck_manager;
}

template<typename T>
OBSManager*
ServiceImpl<T>::obsManager() const {
	return _obs_manager;
}