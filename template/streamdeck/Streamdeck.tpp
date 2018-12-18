/*
 * Plugin Includes
 */
#include "include/streamdeck/Streamdeck.hpp"
#include "include/common/Logger.hpp"
#include "include/common/Global.h"

/*
========================================================================================================
	JSON Helpers
========================================================================================================
*/

template<typename T>
inline bool
rpc2json(QJsonObject& response, const T& data) {
	Q_UNUSED(response);
	Q_UNUSED(data);

	logError("Can't convert data to JSON (type unknown)");

	return false;
}

template<>
inline bool
rpc2json(QJsonObject& response, const std::string& data) {
	Streamdeck::addToJsonObject(response["result"], "data", data.c_str());
	return true;
}

template<>
inline bool
rpc2json(QJsonObject& response, const CollectionPtr& data) {
	QJsonObject data_json;
	Streamdeck::addToJsonObject(data_json, "id", QString("%1").arg(data->id()));
	Streamdeck::addToJsonObject(data_json, "name", data->name().c_str());
	Streamdeck::addToJsonObject(response["result"], "data", data_json);
	return true;
}

/*
========================================================================================================
	RPC Protocol
========================================================================================================
*/

template<typename T>
bool
Streamdeck::sendEvent(const rpc_event event, const T& data, bool event_mode) {
	if(m_subscribedResources.find(event) == m_subscribedResources.end())
		return false;

	QJsonObject response = buildJsonResult(
		rpc_event::NO_EVENT,
		QString::fromStdString(m_subscribedResources[event]),
		event_mode
	);

	bool converted = rpc2json(response, data);

	if(converted) {
		log_custom(LOG_STREAMDECK) << QString("Send Event Message to %1.")
			.arg(m_subscribedResources[event].c_str())
			.toStdString();
		send(event, QJsonDocument(response));
	}

	return converted;
}