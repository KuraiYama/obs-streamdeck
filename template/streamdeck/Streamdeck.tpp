/*
 * Plugin Includes
 */
#include "include/streamdeck/Streamdeck.hpp"
#include "include/common/Logger.hpp"
#include "include/common/Global.h"

/*
========================================================================================================
	RPC Protocol
========================================================================================================
*/

template<typename T>
bool
Streamdeck::sendEvent(const rpc_event event, const T& data) {
	Q_UNUSED(event);
	Q_UNUSED(data);
	log_error << "[Streamdeck] Event parameters list not handled.";
	return false;
}

template<>
inline bool
Streamdeck::sendEvent(const rpc_event event, const std::string& data) {
	if(m_subscribedResources.find(event) == m_subscribedResources.end())
		return false;

	QJsonObject response = buildJsonResult(
		rpc_event::NO_EVENT,
		QString::fromStdString(m_subscribedResources[event])
	);

	addToJsonObject(response["result"], "data", data.c_str());

	log_custom(LOG_STREAMDECK) << QString("Send Event Message to %1.")
		.arg(m_subscribedResources[event].c_str())
		.toStdString();

	emit write(QJsonDocument(response));
	return true;
}