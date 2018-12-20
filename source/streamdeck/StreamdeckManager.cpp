/*
 * Plugin Includes
 */
#include "include/common/SharedVariables.hpp"
#include "include/streamdeck/StreamdeckManager.hpp"
#include "include/common/Logger.hpp"

/*
========================================================================================================
	Constructor / Destructors
========================================================================================================
*/

StreamdeckServer::StreamdeckServer(QObject* parent) :
	QTcpServer(parent) {
	log_info << "[Streamdeck Server] Ready.";
}

StreamdeckServer::~StreamdeckServer() {
	close();
	log_info << "[Streamdeck Server] Close.";
}

StreamdeckManager::StreamdeckManager(short listen_port) : 
	m_internalServer(this) {
	for(int i = 1; i < (int)Streamdeck::rpc_event::COUNT; i++)
		this->addEvent((Streamdeck::rpc_event)i);
	
	m_internalServer.connect(&m_internalServer, &StreamdeckServer::newConnection, this, 
		&StreamdeckManager::onClientConnected);

	m_internalServer.listen(QHostAddress::LocalHost, listen_port);

	log_custom(LOG_STREAMDECK_MANAGER) << "[Streamdeck Manager] Server is listening.";
}

StreamdeckManager::~StreamdeckManager() {
	for(int i = 1; i < (int)Streamdeck::rpc_event::COUNT; i++)
		this->removeEvent((Streamdeck::rpc_event)i);

	for(auto i = m_streamdecks.begin(); i != m_streamdecks.end();) {
		Streamdeck* cl = *i;
		i++;
		close(cl);
	}

	m_streamdecks.clear();

	m_internalServer.disconnect(&m_internalServer, &StreamdeckServer::newConnection, this,
		&StreamdeckManager::onClientConnected);

	m_internalServer.close();
}

/*
========================================================================================================
	Connection Management
========================================================================================================
*/

void
StreamdeckServer::incomingConnection(qintptr socketDescriptor) {
	log_info << "[Streamdeck Server] New incoming connection.";
	StreamdeckClient* client = Streamdeck::createClient(socketDescriptor);
	if(client != nullptr) {
		log_info << "[Streamdeck Server] Client created, add to pending list.";
		QTcpSocket* socket = client->socket();
		m_pendingClients[socket] = client;
		addPendingConnection(socket);
	}
}

StreamdeckClient*
StreamdeckServer::nextPendingClient() {
	if(!this->hasPendingConnections())
		return nullptr;

	QTcpSocket* socket = this->nextPendingConnection();
	auto client_info = m_pendingClients.find(socket);
	return client_info != m_pendingClients.end() ? client_info.value() : nullptr;
}

void
StreamdeckManager::onClientConnected() {
	StreamdeckClient* client = m_internalServer.nextPendingClient();
	
	if(client != nullptr) {
		log_custom(LOG_STREAMDECK_MANAGER) << "[Streamdeck Manager] New client is ready."
			" Creating the streamdeck.";
		Streamdeck* streamdeck = new Streamdeck(*client);
		connect(streamdeck, &Streamdeck::clientDisconnected, this,
			&StreamdeckManager::onClientDisconnected);
		connect(streamdeck, &Streamdeck::received, this,
			&StreamdeckManager::receiveMessage);
		m_streamdecks.insert(streamdeck);

		client->ready();
	}
}

void
StreamdeckManager::onClientDisconnected(Streamdeck* streamdeck, int code) {
	log_warn << QString("[Streamdeck Manager] Streamdeck disconnected (%1). Deleting it.")
		.arg(code).toStdString();
	m_streamdecks.remove(streamdeck);
	disconnect(streamdeck, &Streamdeck::clientDisconnected, this,
		&StreamdeckManager::onClientDisconnected);
	disconnect(streamdeck, &Streamdeck::received, this,
		&StreamdeckManager::receiveMessage);
	delete streamdeck;
}

/*
========================================================================================================
	Streamdeck Management
========================================================================================================
*/

void
StreamdeckManager::close(Streamdeck* streamdeck) {
	streamdeck->close();
}

bool
StreamdeckManager::setAcknowledge(Streamdeck* client, const rpc_adv_response<void>& response) {
	QString resource = response.request != nullptr ?
		QString("%1.%2")
		.arg(response.request->serviceName.c_str())
		.arg(response.request->method.c_str()) :
		QString("%1.%2")
		.arg(response.serviceName)
		.arg(response.method);

	return client->sendAcknowledge(response.event, resource.toStdString());
}

bool
StreamdeckManager::setSubscription(Streamdeck* client, const rpc_adv_response<std::string>& response) {
	return client->sendSubscription(response.event, response.data);
}

bool
StreamdeckManager::setError(Streamdeck* client, const rpc_adv_response<bool>& response) {
	QString resource = response.request != nullptr ?
		QString("%1.%2")
			.arg(response.request->serviceName.c_str())
			.arg(response.request->method.c_str()) :
		QString("%1.%2")
			.arg(response.serviceName)
			.arg(response.method);

	return client->sendError(response.event, resource.toStdString(), response.data);
}

bool
StreamdeckManager::setRecordStreamState(
	Streamdeck* client, 
	const rpc_adv_response<std::pair<std::string, std::string>>& response
) {
	QString resource = QString("%1.%2")
		.arg(response.request->serviceName.c_str())
		.arg(response.request->method.c_str());

	return client->sendRecordStreamState(
		response.event,
		resource.toStdString(),
		response.data.first,
		response.data.second
	);
}

bool
StreamdeckManager::setSchema(Streamdeck* client, const rpc_adv_response<Collections>& response) {
	QString resource = response.request != nullptr ?
		QString("%1.%2")
		.arg(response.request->serviceName.c_str())
		.arg(response.request->method.c_str()) :
		QString("%1.%2")
		.arg(response.serviceName)
		.arg(response.method);

	return client->sendSchemaMessage(response.event, resource.toStdString(), response.data);
}

bool
StreamdeckManager::setCollections(Streamdeck* client, const rpc_adv_response<Collections>& response) {
	QString resource = response.request != nullptr ?
		QString("%1.%2")
			.arg(response.request->serviceName.c_str())
			.arg(response.request->method.c_str()) :
		QString("%1.%2")
			.arg(response.serviceName)
			.arg(response.method);

	return client->sendCollectionsMessage(response.event, resource.toStdString(), response.data);
}

bool
StreamdeckManager::setCollection(Streamdeck* client, const rpc_adv_response<CollectionPtr>& response) {
	QString resource = response.request != nullptr ?
		QString("%1.%2")
		.arg(response.request->serviceName.c_str())
		.arg(response.request->method.c_str()) :
		QString("%1.%2")
		.arg(response.serviceName)
		.arg(response.method);

	return client->sendCollectionMessage(response.event, resource.toStdString(), response.data);
}

bool
StreamdeckManager::setScenes(Streamdeck* client, const rpc_adv_response<Scenes>& response) {
	QString resource = response.request != nullptr ?
		QString("%1.%2")
		.arg(response.request->serviceName.c_str())
		.arg(response.request->method.c_str()) :
		QString("%1.%2")
		.arg(response.serviceName)
		.arg(response.method);

	return client->sendScenesMessage(response.event, resource.toStdString(), response.data);
}

bool
StreamdeckManager::setScene(Streamdeck* client, const rpc_adv_response<ScenePtr>& response) {
	QString resource = response.request != nullptr ?
		QString("%1.%2")
		.arg(response.request->serviceName.c_str())
		.arg(response.request->method.c_str()) :
		QString("%1.%2")
		.arg(response.serviceName)
		.arg(response.method);

	return client->sendSceneMessage(response.event, resource.toStdString(), response.data);
}

/*
bool
StreamdeckManager::setScenes(
	Streamdeck* client,
	const rpc_adv_response<std::tuple<Collection*,Scenes>>& response
) {
	QString resource = response.request != nullptr ?
		QString("%1.%2")
		.arg(response.request->serviceName.c_str())
		.arg(response.request->method.c_str()) :
		QString("%1.%2")
		.arg(response.serviceName)
		.arg(response.method);

	return client->sendScenesMessage(response.event, resource.toStdString(), 
		std::get<0>(response.data), std::get<1>(response.data));
}
*/

/*
========================================================================================================
	Messages Handling
========================================================================================================
*/

bool
StreamdeckManager::validate(rpc_response& response) {
	bool result = (response.request == nullptr) ||
		(response.request != nullptr && response.event == response.request->event);
	if(!result) {
		log_error << QString("Service %1::%2 could not process the requested event %3.")
			.arg(response.serviceName)
			.arg(response.method)
			.arg((int)response.request->event).toStdString();
		if(response.request != nullptr)
			response.event = response.request->event;
	}
	return result;
}

void
StreamdeckManager::receiveMessage(
	Streamdeck* streamdeck,
	Streamdeck::rpc_event event, 
	QString service,
	QString method,
	QVector<QVariant> args,
	bool& error
) {
	if(!m_streamdecks.contains(streamdeck))
		return;

	log_custom(LOG_STREAMDECK_MANAGER) << "[Streamdeck Manager] Message received. "
		"Dispatching to services.";

	error = !this->notifyEvent<const rpc_event_data&>(event, 
		(rpc_event_data { event, streamdeck, service.toStdString(), method.toStdString(), args }));

	if(error) {
		log_error << "[Streamdeck Manager] Error when processing messages.";
		close(streamdeck);
		m_streamdecks.remove(streamdeck);
	}
}