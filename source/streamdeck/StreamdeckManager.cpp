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
	log_info << "[Streamdeck Server] Ready." << log_end;
}

StreamdeckServer::~StreamdeckServer() {
	close();
	log_info << "[Streamdeck Server] Close." << log_end;
}

StreamdeckManager::StreamdeckManager() : 
	m_internalServer(this) {
	for(int i = 1; i < (int)rpc::event::COUNT; i++)
		this->addEvent((rpc::event)i);
	
	m_internalServer.connect(&m_internalServer, &StreamdeckServer::newConnection, this, 
		&StreamdeckManager::onClientConnected);
}

StreamdeckManager::~StreamdeckManager() {
	for(int i = 1; i < (int)rpc::event::COUNT; i++)
		this->removeEvent((rpc::event)i);

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
StreamdeckManager::listen(short listen_port) {
	m_internalServer.listen(QHostAddress::LocalHost, listen_port);
	log_custom(LOG_STREAMDECK_MANAGER) << "[Streamdeck Manager] Server is listening." << log_end;
}

void
StreamdeckServer::incomingConnection(qintptr socketDescriptor) {
	log_info << "[Streamdeck Server] New incoming connection." << log_end;
	StreamdeckClient* client = Streamdeck::createClient(socketDescriptor);
	if(client != nullptr) {
		log_info << "[Streamdeck Server] Client created, add to pending list." << log_end;
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
			" Creating the streamdeck." << log_end;
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
		.arg(code).toStdString() << log_end;
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

QString
StreamdeckManager::formatResource(const rpc::response_base& response) {
	if(response.request == nullptr) {
		return QString("%1.%2").arg(response.serviceName).arg(response.method);
	}
	else {
		const char* service = response.request->serviceName.c_str();
		if(strlen(service) == 0)
			service = response.serviceName;
		const char* method = response.request->method.c_str();
		if(strlen(method) == 0)
			method = response.method;

		return QString("%1.%2").arg(service).arg(method);
	}
}

bool
StreamdeckManager::setAcknowledge(Streamdeck* client, const rpc::response<void>& response) {
	QString resource = formatResource(response);
	return client->sendAcknowledge(response.event, resource.toStdString());
}

bool
StreamdeckManager::setSubscription(Streamdeck* client, const rpc::response<std::string>& response) {
	return client->sendSubscription(response.event, response.data);
}

bool
StreamdeckManager::setError(Streamdeck* client, const rpc::response<rpc::response_error>& response) {
	QString resource = formatResource(response);
	return client->sendError(response.event, resource.toStdString(), response.data);
}

bool
StreamdeckManager::setRecordStreamState(
	Streamdeck* client, 
	const rpc::response<std::pair<std::string, std::string>>& response
) {
	QString resource = formatResource(response);
	return client->sendRecordStreamState(
		response.event,
		resource.toStdString(),
		response.data.first,
		response.data.second
	);
}

bool
StreamdeckManager::setSchema(Streamdeck* client, const rpc::response<Collections>& response) {
	QString resource = formatResource(response);
	return client->sendSchema(response.event, resource.toStdString(), response.data);
}

bool
StreamdeckManager::setCollections(Streamdeck* client, const rpc::response<Collections>& response) {
	QString resource = formatResource(response);
	return client->sendCollections(response.event, resource.toStdString(), response.data);
}

bool
StreamdeckManager::setCollection(Streamdeck* client, const rpc::response<CollectionPtr>& response) {
	QString resource = formatResource(response);
	return client->sendCollection(response.event, resource.toStdString(), response.data);
}

bool
StreamdeckManager::setScenes(Streamdeck* client, const rpc::response<Scenes>& response) {
	QString resource = formatResource(response);
	return client->sendScenes(response.event, resource.toStdString(), response.data);
}

bool
StreamdeckManager::setScene(Streamdeck* client, const rpc::response<ScenePtr>& response) {
	QString resource = formatResource(response);
	return client->sendScene(response.event, resource.toStdString(), response.data);
}

bool
StreamdeckManager::setSources(Streamdeck* client, const rpc::response<Sources>& response) {
	QString resource = formatResource(response);
	return client->sendSources(response.event, resource.toStdString(), response.data);
}

/*
========================================================================================================
	Messages Handling
========================================================================================================
*/

bool
StreamdeckManager::validate(rpc::response_base& response) {
	bool result = (response.request == nullptr) ||
		(response.request != nullptr && response.event == response.request->event);
	if(!result) {
		log_error << QString("Service %1::%2 could not process the requested event %3.")
			.arg(response.serviceName)
			.arg(response.method)
			.arg((int)response.request->event).toStdString() << log_end;
		if(response.request != nullptr)
			response.event = response.request->event;
	}
	return result;
}

void
StreamdeckManager::receiveMessage(
	Streamdeck* streamdeck,
	rpc::event event, 
	QString service,
	QString method,
	QVector<QVariant> args,
	bool& error
) {
	if(!m_streamdecks.contains(streamdeck))
		return;

	log_custom(LOG_STREAMDECK_MANAGER) << "[Streamdeck Manager] Message received. "
		"Dispatching to services." << log_end;

	error = !this->notifyEvent<const rpc::request&>(event, 
		(rpc::request { event, streamdeck, service.toStdString(), method.toStdString(), args }));

	if(error) {
		log_error << "[Streamdeck Manager] Error when processing messages." << log_end;
		close(streamdeck);
		m_streamdecks.remove(streamdeck);
	}
}