/*
 * Plugin Includes
 */
#include "include/services/Service.hpp"
#include "include/Global.h"
#include "include/common/Logger.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

template<typename T>
ServiceT<T>::ServiceT(const char* local_name, const char* remote_name) : 
	Service(local_name, remote_name) {
	static_assert(std::is_base_of<ServiceT<T>, T>::value);
}

template<typename T>
ServiceT<T>::~ServiceT() {
}

/*
========================================================================================================
	Events Handling
========================================================================================================
*/

template<typename T>
void
ServiceT<T>::setupEvent(obs_frontend_event event, obs_frontend_callback handler) {
	m_frontendEvent.addEvent(event);
	this->EventObserver<T, obs_frontend_event>::registerCallback(
		event,
		(obs_frontend_callback)handler, 
		reinterpret_cast<T*>(this)
	);
	m_frontendEvent.addEventHandler(event, this);
}

template<typename T>
void
ServiceT<T>::setupEvent(
	Streamdeck::rpc_event event, 
	typename RPCHandler::template FuncWrapperB<void>::Callback handler
) {
	if(_streamdeck_manager == nullptr)
		return;

	_rpcEvent.registerCallback(
		event, 
		(RPCHandler::FuncWrapperB<void>::Callback)handler, reinterpret_cast<T*>(this)
	);
	_streamdeck_manager->addEventHandler(event, &m_rpcEvent);
}

template<typename T>
template<typename B>
void
ServiceT<T>::setupEvent(
	Streamdeck::rpc_event event, 
	typename RPCHandler::template FuncWrapperB<B>::Callback handler
) {
	if(_streamdeck_manager == nullptr)
		return;

	m_rpcEvent.registerCallback<B>(
		event, 
		(RPCHandler::FuncWrapperB<B>::Callback)handler, reinterpret_cast<T*>(this)
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
ServiceT<T>::logInfo(const std::string& message) const {
	log_info << QString("[%1] %2")
		.arg(QString(m_localName))
		.arg(QString::fromStdString(message))
		.toStdString();
}

template<typename T>
void
ServiceT<T>::logError(const std::string& message) const {
	log_error << QString("[%1] %2")
		.arg(QString(m_localName))
		.arg(QString::fromStdString(message))
		.toStdString();
}

template<typename T>
void
ServiceT<T>::logWarning(const std::string& message) const {
	log_warn << QString("[%1] %2")
		.arg(QString(m_localName))
		.arg(QString::fromStdString(message))
		.toStdString();
}

/*
========================================================================================================
	RPC Helpers
========================================================================================================
*/

template<typename T>
bool
ServiceT<T>::checkResource(const rpc_event_data* data, const QRegExp& method) const {
	return data == nullptr ||
		(data->serviceName.compare(m_remoteName) == 0 && method.exactMatch(data->method.c_str()));
}

/*
========================================================================================================
	RPC Response Initialiaztion
========================================================================================================
*/

template<typename T>
rpc_adv_response<void>
ServiceT<T>::response_void(const rpc_event_data* data, const char* method) const {
	return
		rpc_adv_response<void>{
			{data, Streamdeck::rpc_event::NO_EVENT, name(), method}
	};
}

template<typename T>
rpc_adv_response<std::string>
ServiceT<T>::response_string(const rpc_event_data* data, const char* method) const {
	return 
		rpc_adv_response<std::string>{ 
			{data, Streamdeck::rpc_event::NO_EVENT, name(), method},
			std::string("") 
		};
}

template<typename T>
rpc_adv_response<bool>
ServiceT<T>::response_bool(const rpc_event_data* data, const char* method) const {
	return
		rpc_adv_response<bool>{
			{data, Streamdeck::rpc_event::NO_EVENT, name(), method},
			true
		};
}

template<typename T>
rpc_adv_response<std::pair<std::string,std::string>>
ServiceT<T>::response_string2(const rpc_event_data* data, const char* method) const {
	return
		rpc_adv_response<std::pair<std::string,std::string>>{
			{data, Streamdeck::rpc_event::NO_EVENT, name(), method},
			std::pair<std::string,std::string>("","")
		};
}

template<typename T>
rpc_adv_response<CollectionPtr>
ServiceT<T>::response_collection(const rpc_event_data* data, const char* method) const {
	return
		rpc_adv_response<Collection*>{
			{data, Streamdeck::rpc_event::NO_EVENT, name(), method},
			nullptr
	};
}

template<typename T>
rpc_adv_response<Collections>
ServiceT<T>::response_collections(const rpc_event_data* data, const char* method) const {
	return
		rpc_adv_response<Collections>{
			{data, Streamdeck::rpc_event::NO_EVENT, name(), method},
			Collections()
		};
}

/*template<typename T>
rpc_adv_response<std::tuple<Collection*, Scenes>>
ServiceT<T>::response_scenes(const rpc_event_data* data, const char* method) const {
	return
		rpc_adv_response<std::tuple<Collection*, Scenes>>{
			{data, Streamdeck::rpc_event::NO_EVENT, name(), method},
			std::tuple<Collection*, Scenes>(nullptr, Scenes())
	};
}*/

/*
========================================================================================================
	Accessors
========================================================================================================
*/

template<typename T>
const char*
ServiceT<T>::name() const {
	return m_localName;
}

template<typename T>
StreamdeckManager*
ServiceT<T>::streamdeckManager() const {
	return _streamdeck_manager;
}

template<typename T>
OBSManager*
ServiceT<T>::obsManager() const {
	return _obs_manager;
}

/*
========================================================================================================
	Operators
========================================================================================================
*/

template<typename T>
ServiceT<T>::operator Service *() const {
	return this;
}