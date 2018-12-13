/*
 * CRT Includes
 */
#include <cstdlib>

/*
 * Qt Includes
 */
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

/*
 * Plugin Includes
 */
#include "include/streamdeck/Streamdeck.hpp"
#include "include/common/SharedVariablesManager.hpp"
#include "include/common/Logger.hpp"

/*
========================================================================================================
	Extern function
========================================================================================================
*/

extern std::string ptr_to_string(void* ptr);

/*
========================================================================================================
	Static Attributes Initializations
========================================================================================================
*/

bool StreamdeckClient::_isVerbose = false;

/*
========================================================================================================
	Constructor / Destructors
========================================================================================================
*/

StreamdeckClient::StreamdeckClient(qintptr socketDescriptor) : m_socketDescriptor(socketDescriptor),
		m_startExecution(false), m_isClosing(false) {

	m_internalSocket = nullptr;
}

StreamdeckClient::~StreamdeckClient() {
	if(m_internalSocket != nullptr) {
		disconnect(m_internalSocket, SIGNAL(disconnected(void)), this, SLOT(disconnected(void)));
		disconnect(m_internalSocket, SIGNAL(readyRead(void)), this, SLOT(read(void)));
		m_internalSocket->deleteLater();
		m_internalSocket = nullptr;
	}
	log_custom(LOG_STREAMDECK_CLIENT) << "[Streamdeck Client] Destroyed.";
}

Streamdeck::Streamdeck(StreamdeckClient& client) : m_internalClient(client) {
	m_internalClient.setParent(this);
	connect(&m_internalClient, SIGNAL(disconnected(int)), this, SLOT(disconnected(int)));
	connect(&m_internalClient, SIGNAL(read(QJsonDocument)), this, SLOT(read(QJsonDocument)));
	connect(this, SIGNAL(write(QJsonDocument, bool&)), &m_internalClient, 
		SLOT(write(QJsonDocument, bool&)));
	connect(this, &Streamdeck::close_client, &m_internalClient, &StreamdeckClient::close);

	for(int i = 1; i < (int)rpc_event::RPC_ID_COUNT; i++) {
		m_authorizedEvents[(rpc_event)i] = true;
	}

	updateEventAuthorizations(rpc_event::RPC_ID_START_STREAMING, false);
	updateEventAuthorizations(rpc_event::RPC_ID_STOP_STREAMING, false);
	updateEventAuthorizations(rpc_event::RPC_ID_START_RECORDING, false);
	updateEventAuthorizations(rpc_event::RPC_ID_STOP_RECORDING, false);
}

Streamdeck::~Streamdeck() {

	log_custom(LOG_STREAMDECK) << "[Streamdeck] Destruction...";

	if(m_internalClient.isRunning()) {
		m_internalClient.exit(0);
	}

	disconnect(&m_internalClient, SIGNAL(disconnected(int)), this, SLOT(disconnected(int)));
	disconnect(&m_internalClient, SIGNAL(read(QJsonDocument)), this, SLOT(read(QJsonDocument)));
	disconnect(this, SIGNAL(write(QJsonDocument, bool&)), &m_internalClient,
		SLOT(write(QJsonDocument, bool&)));
	disconnect(this, &Streamdeck::close_client, &m_internalClient, &StreamdeckClient::close);

	delete &m_internalClient;
}

/*
========================================================================================================
	Thread Handling
========================================================================================================
*/

void StreamdeckClient::run() {

	log_custom(LOG_STREAMDECK_CLIENT) << "[Streamdeck Client] New thread run.";

	bool_s ready = shared_variable<bool>("ready");
	bool_s socket_created = shared_variable<bool>("socket_created");

	ready.wait([](const bool& value){ return value == true; });

	m_internalSocket = new QTcpSocket(this);

	if(!m_internalSocket->setSocketDescriptor(m_socketDescriptor)) {
		m_internalSocket->close();
		m_internalSocket->deleteLater();
		m_internalSocket = nullptr;
		m_socketDescriptor = -1;
	}

	log_custom(LOG_STREAMDECK_CLIENT) << "[Streamdeck Client] Socket created.";

	if(m_internalSocket != nullptr) {
		connect(m_internalSocket, SIGNAL(disconnected(void)), this, SLOT(disconnected(void)));
		connect(m_internalSocket, SIGNAL(readyRead(void)), this, SLOT(read(void)));
	}

	socket_created = true;

	if(m_internalSocket == nullptr) 
		return;

	while(!m_startExecution);

	/*****************************/
	//        EVENT LOOP         //
	///////////////////////////////
	int result = exec();
	/****************************/
}

/*
========================================================================================================
	JSON Helpers
========================================================================================================
*/

QJsonObject Streamdeck::buildJsonResult(const rpc_event event, const QString& resourceId) const {
	QJsonObject response, result;
	response["jsonrpc"] = "2.0";
	response["id"] = (int)event;
	result["resourceId"] = resourceId;
	if(event == rpc_event::RPC_ID_NO_EVENT)
		result["_type"] = "EVENT";
	response["result"] = result;
	return response;
}

QJsonObject Streamdeck::buildJsonResponse(const rpc_event event, const QString& resourceId) const {
	QJsonObject response;
	response["jsonrpc"] = "2.0";
	response["id"] = (int)event;
	response["resourceId"] = resourceId;
	if(event == rpc_event::RPC_ID_NO_EVENT)
		response["_type"] = "EVENT";
	return response;
}

void Streamdeck::addToJsonObject(QJsonObject& json_object, QString key, QJsonValue&& value) const {
	json_object[key] = value;
}

void Streamdeck::addToJsonObject(QJsonValueRef&& json_object, QString key, QJsonValue&& value) const {
	if(json_object.isObject() == true) {
		QJsonObject copy(json_object.toObject());
		copy[key] = value;
		json_object = copy;
	}
}

void Streamdeck::addToJsonArray(QJsonArray& json_array, QJsonValue&& value) const {
	json_array.append(value);
}

void Streamdeck::addToJsonArray(QJsonValueRef&& json_array, QJsonValue&& value) const {
	if(json_array.isArray() == true) {
		QJsonArray copy(json_array.toArray());
		copy.append(value);
		json_array = copy;
	}
}

/*
========================================================================================================
	RPC Protocol
========================================================================================================
*/

void Streamdeck::updateEventAuthorizations(const rpc_event event, bool value) {
	m_authorizedEvents[event] = value;
	switch(event) {
		case rpc_event::RPC_ID_START_STREAMING:
		case rpc_event::RPC_ID_STOP_STREAMING:
			m_authorizedEvents[rpc_event::RPC_ID_STREAMING_STATUS_CHANGED_SUBSCRIBE] = !value;
			break;

		case rpc_event::RPC_ID_START_RECORDING:
		case rpc_event::RPC_ID_STOP_RECORDING:
			m_authorizedEvents[rpc_event::RPC_ID_RECORDING_STATUS_CHANGED_SUBSCRIBE] = !value;
			break;
			
		case rpc_event::RPC_ID_FETCH_COLLECTIONS_SCHEMA:
			m_authorizedEvents[rpc_event::RPC_ID_COLLECTION_ADDED_SUBSCRIBE] = !value;
			m_authorizedEvents[rpc_event::RPC_ID_COLLECTION_REMOVED_SUBSCRIBE] = !value;
			m_authorizedEvents[rpc_event::RPC_ID_COLLECTION_UPDATED_SUBSCRIBE] = !value;
			m_authorizedEvents[rpc_event::RPC_ID_COLLECTION_SWITCHED_SUBSCRIBE] = !value;
			m_authorizedEvents[rpc_event::RPC_ID_GET_ACTIVE_COLLECTION] = !value;
			m_authorizedEvents[rpc_event::RPC_ID_MAKE_COLLECTION_ACTIVE] = !value;
			m_authorizedEvents[rpc_event::RPC_ID_GET_SCENES] = !value;
			break;

		case rpc_event::RPC_ID_GET_COLLECTIONS:
			m_authorizedEvents[rpc_event::RPC_ID_COLLECTION_ADDED_SUBSCRIBE] = value;
			m_authorizedEvents[rpc_event::RPC_ID_COLLECTION_REMOVED_SUBSCRIBE] = value;
			m_authorizedEvents[rpc_event::RPC_ID_COLLECTION_UPDATED_SUBSCRIBE] = value;
			m_authorizedEvents[rpc_event::RPC_ID_COLLECTION_SWITCHED_SUBSCRIBE] = value;
			m_authorizedEvents[rpc_event::RPC_ID_GET_ACTIVE_COLLECTION] = value;
			m_authorizedEvents[rpc_event::RPC_ID_MAKE_COLLECTION_ACTIVE] = value;
			m_authorizedEvents[rpc_event::RPC_ID_GET_SCENES] = value;
			break;

		default:
			break;
	}
}

bool Streamdeck::sendSubscriptionMessage(const rpc_event event, const std::string& resourceId) {
	
	log_custom(LOG_STREAMDECK) << "Event Resource subscribed.";

	m_subscribedResources[event] = resourceId;
	QJsonObject response = buildJsonResult(event, QString::fromStdString(resourceId));
	bool result = false;
	emit write(QJsonDocument(response), result);
	return true;
}

bool Streamdeck::sendStatusMessage(const rpc_event event, const std::string& status) {
	if(m_subscribedResources.find(event) == m_subscribedResources.end())
		return false;

	if(m_authorizedEvents[event] == false) 
		return true;

	log_custom(LOG_STREAMDECK) << "Event Message sent.";

	QJsonObject response = buildJsonResult(rpc_event::RPC_ID_NO_EVENT, 
		QString::fromStdString(m_subscribedResources[event]));
	addToJsonObject(response["result"], "data", status.c_str());
	bool result = false;
	emit write(QJsonDocument(response), result);
	updateEventAuthorizations(event, true);
	return true;
}

bool Streamdeck::sendStatusMessage(const rpc_event event) {
	if(m_authorizedEvents[event] == false)
		return true;

	if(m_subscribedResources.find(event) == m_subscribedResources.end())
		return false;

	log_custom(LOG_STREAMDECK) << "Event Message sent.";

	QJsonObject response = buildJsonResult(rpc_event::RPC_ID_NO_EVENT,
		QString::fromStdString(m_subscribedResources[event]));
	bool result = false;
	emit write(QJsonDocument(response), result);
	updateEventAuthorizations(event, true);
	return true;
}

bool Streamdeck::sendRecordStreamState(const rpc_event event, const std::string& resourceId,
		const std::string& streaming, const std::string& recording) {

	log_custom(LOG_STREAMDECK) << "Recording and Streaming status sent.";

	QJsonObject response = buildJsonResult(event, QString::fromStdString(resourceId));
	addToJsonObject(response["result"], "streamingStatus", streaming.c_str());
	addToJsonObject(response["result"], "recordingStatus", recording.c_str());
	bool result = false;
	emit write(QJsonDocument(response), result);
	return true;
}

bool Streamdeck::sendErrorMessage(const rpc_event event, const std::string& resourceId, bool error) {
	if(m_authorizedEvents[event] == false)
		return true;

	log_custom(LOG_STREAMDECK) << "Error Message sent.";

	QJsonObject response = buildJsonResult(event, QString::fromStdString(resourceId));
	this->addToJsonObject(response, "error", error);
	bool result = false;
	emit this->write(QJsonDocument(response), result);
	updateEventAuthorizations(event, false);
	return true;
}

bool Streamdeck::sendActiveCollectionMessage(const rpc_event event, const std::string& resourceId,
		const Collection* collection) {

	if(m_authorizedEvents[event] == false)
		return true;

	log_custom(LOG_STREAMDECK) << "Active collection sent.";

	QJsonObject response = buildJsonResult(event, QString::fromStdString(resourceId));
	QString id = QString::fromStdString(
		std::to_string((reinterpret_cast<unsigned long long>(collection))));
	this->addToJsonObject(response["result"], "id", id);
	bool result = false;
	emit this->write(QJsonDocument(response), result);
	return true;
}

bool Streamdeck::sendCollectionSwitchMessage(const rpc_event event, const Collection* collection) {
	if(m_authorizedEvents[event] == false)
		return true;

	if(m_subscribedResources.find(event) == m_subscribedResources.end())
		return false;

	log_custom(LOG_STREAMDECK) << "Switch collection message sent.";

	QJsonObject response = buildJsonResult(rpc_event::RPC_ID_NO_EVENT,
		QString::fromStdString(m_subscribedResources[event]));
	QString id = QString::fromStdString(
		std::to_string((reinterpret_cast<unsigned long long>(collection))));
	QJsonObject data;
	addToJsonObject(data, "id", id);
	addToJsonObject(response["result"], "data", data);
	bool result = false;
	emit write(QJsonDocument(response), result);
	updateEventAuthorizations(event, true);
	return true;
}

bool Streamdeck::sendCollectionsMessage(const rpc_event event, const std::string& resourceId,
		const Collections& collections) {

	if(m_authorizedEvents[event] == false)
		return true;

	rpc_event event_to_send = event;
	std::string resource = resourceId;

	if(event_to_send != rpc_event::RPC_ID_GET_COLLECTIONS) {
		if(m_subscribedResources.find(event_to_send) == m_subscribedResources.end())
			return false;
		resource = m_subscribedResources[event_to_send];
		event_to_send = rpc_event::RPC_ID_NO_EVENT;
	}

	log_custom(LOG_STREAMDECK) << "Collections sent.";

	QJsonObject response = buildJsonResult(event_to_send, QString::fromStdString(resource));

	QJsonArray data;
	for(auto iter = collections.begin(); iter < collections.end(); iter++) {
		QJsonObject collection;
		addToJsonObject(collection, "name", (*iter)->name().c_str());
		addToJsonObject(collection, "id", (*iter)->id().c_str());
		addToJsonArray(data, collection);
	}
	addToJsonObject(response["result"], "data", data);

	bool result = false;
	emit this->write(QJsonDocument(response), result);

	updateEventAuthorizations(rpc_event::RPC_ID_GET_COLLECTIONS, true);

	return true;
}

bool Streamdeck::sendCollectionsSchema(const rpc_event event, const Collections& collections) {

	if(m_authorizedEvents[event] == false)
		return true;

	rpc_event event_to_send = event;
	if(m_subscribedResources.find(event_to_send) == m_subscribedResources.end())
		return false;

	log_custom(LOG_STREAMDECK) << "Collection schema sent.";

	std::string resource = m_subscribedResources[event_to_send];

	if(event_to_send == rpc_event::RPC_ID_FETCH_COLLECTIONS_SCHEMA)
		event_to_send = rpc_event::RPC_ID_GET_COLLECTIONS;

	QJsonObject response = buildJsonResult(event_to_send, QString::fromStdString(resource));

	addToJsonObject(response["result"], "_type", "EVENT");
	QJsonArray data_collections;
	for(auto iter_cl = collections.begin(); iter_cl < collections.end(); iter_cl++) {

		Collection* collection_ptr = *iter_cl;

		QJsonObject collection_json;
		addToJsonObject(collection_json, "name", collection_ptr->name().c_str());
		addToJsonObject(collection_json, "id", collection_ptr->id().c_str());

		QJsonArray data_scenes;
		Scenes scenes = collection_ptr->scenes();
		for(auto iter_sc = scenes.begin(); iter_sc < scenes.end(); iter_sc++) {

			Scene* scene_ptr = *iter_sc;

			QJsonObject scene_json;
			addToJsonObject(scene_json, "name", scene_ptr->name().c_str());
			addToJsonObject(scene_json, "id", scene_ptr->id().c_str());

			QJsonArray data_items;
			Items items = scene_ptr->items();
			for(auto iter_it = items.begin(); iter_it < items.end(); iter_it++) {

				Item* item_ptr = *iter_it;

				QJsonObject item_json;
				addToJsonObject(item_json, "sceneItemId", item_ptr->id());
				addToJsonObject(item_json, "sourceId", item_ptr->name().c_str());
				addToJsonObject(item_json, "visible", item_ptr->visible());

				addToJsonArray(data_items, item_json);
			}
			addToJsonObject(scene_json, "sceneItems", data_items);

			addToJsonArray(data_scenes, scene_json);
			
		}
		addToJsonObject(collection_json, "scenes", data_scenes);

		addToJsonArray(data_collections, collection_json);
	}
	addToJsonObject(response["result"], "data", data_collections);

	bool result = false;
	emit this->write(QJsonDocument(response), result);

	updateEventAuthorizations(rpc_event::RPC_ID_GET_COLLECTIONS, true);

	return true;
}

bool Streamdeck::sendScenesMessage(const rpc_event event, const std::string& resourceId,
		const Collection* collection, const Scenes& scenes) {

	if(m_authorizedEvents[event] == false)
		return true;

	if(collection == nullptr)
		return true;

	log_custom(LOG_STREAMDECK) << "Scenes sent.";

	QJsonObject response = buildJsonResponse(event, QString::fromStdString(resourceId));

	addToJsonObject(response, "collection", collection->name().c_str());

	QJsonArray data;
	for(auto iter_sc = scenes.begin(); iter_sc < scenes.end(); iter_sc++) {

		Scene* scene_ptr = *iter_sc;

		QJsonObject scene_json;
		addToJsonObject(scene_json, "name", scene_ptr->name().c_str());
		addToJsonObject(scene_json, "id", scene_ptr->id().c_str());

		QJsonArray items_json;
		
		Items items = scene_ptr->items();
		for(auto iter_it = items.begin(); iter_it < items.end(); iter_it++) {
			Item* item_ptr = *iter_it;

			QJsonObject item_json;
			addToJsonObject(item_json, "sceneItemId", item_ptr->id());
			addToJsonObject(item_json, "sourceId", item_ptr->completeName().c_str());
			addToJsonObject(item_json, "visible", item_ptr->visible());
			addToJsonObject(item_json, "sceneNodeType", item_ptr->type());

			addToJsonArray(items_json, item_json);
		}

		/* We can put it in "items" or "nodes" */
		//addToJsonObject(scene_json, "nodes", items_json);
		addToJsonObject(scene_json, "items", items_json);
		addToJsonArray(data, scene_json);
	}
	addToJsonObject(response, "result", data);

	bool result = false;
	emit this->write(QJsonDocument(response), result);

	updateEventAuthorizations(rpc_event::RPC_ID_GET_COLLECTIONS, true);

	return true;
}

/*
========================================================================================================
	Read / Write Operations
========================================================================================================
*/

void StreamdeckClient::read() {
	while(m_internalSocket != nullptr && m_internalSocket->canReadLine()) {
		try {
			log_custom(LOG_STREAMDECK_CLIENT) << "[Streamdeck Client] Read message...";
			QByteArray data = m_internalSocket->readLine();
			QJsonDocument json_quest = QJsonDocument::fromJson(data);
			if(_isVerbose) {
				std::string log = json_quest.toJson(QJsonDocument::JsonFormat::Indented).toStdString();
				log_custom(LOG_STREAMDECK_CLIENT) << log;
			}
			emit read(json_quest);
		}
		catch(...) {
			m_internalSocket->abort();
		}
	}
}

void StreamdeckClient::write(QJsonDocument document, bool& result) {

	log_custom(LOG_STREAMDECK_CLIENT) << QString("[Streamdeck Client] Write message...").toStdString();
	if(_isVerbose) {
		std::string log = document.toJson(QJsonDocument::JsonFormat::Indented).toStdString();
		log_custom(LOG_STREAMDECK_CLIENT) << log;
	}

	QByteArray data = document.toJson(QJsonDocument::JsonFormat::Compact).append("\n");
	if(m_internalSocket != nullptr && m_internalSocket->isValid()) {
		result = m_internalSocket->write(data) == data.length();
	}

	if(!result)
		m_internalSocket->close();
}

void Streamdeck::read(QJsonDocument json_quest) {

	if(!m_internalClient.isRunning())
		return;

	log_custom(LOG_STREAMDECK) << "[Streamdeck] Internal client notified a new message.";

	Streamdeck::rpc_event event;
	QString service, method;
	QVector<QVariant> args;

	this->parse(json_quest, event, service, method, args);

	updateEventAuthorizations(event, true);
	bool error = true;
	emit received(this, event, service, method, args, error);
	if(error) close();
}

void Streamdeck::parse(const QJsonDocument& json_quest, Streamdeck::rpc_event& event, QString& service, 
		QString& method, QVector<QVariant>& args) {

	log_info << "[Streamdeck]Parsing message...";

	event = (rpc_event)(json_quest["id"].isUndefined() ? rpc_event::RPC_ID_ERROR : 
		json_quest["id"].toInt() >= (int)rpc_event::RPC_ID_COUNT ? rpc_event::RPC_ID_ERROR : 
		rpc_event(json_quest["id"].toInt()));

	method = json_quest["method"].toString();

	QJsonObject params = json_quest["params"].toObject();
	service = params["resource"].toString();

	if(params.find("args") != params.end() && params["args"].isArray()) {
		QJsonArray json_args = params["args"].toArray();
		args = QVector<QVariant>::fromList(json_args.toVariantList());
	}

	switch(event) {
		case rpc_event::RPC_ID_START_RECORDING:
			log_custom(0x33ff02) << QString("Action START_RECORD (%1)").arg((int)event).toStdString();
			break;
		case rpc_event::RPC_ID_STOP_RECORDING:
			log_custom(0x33ff02) << QString("Action STOP_RECORD (%1)").arg((int)event).toStdString();
			break;
		case rpc_event::RPC_ID_START_STREAMING:
			log_custom(0x33ff02) << QString("Action START_STREAM (%1)").arg((int)event).toStdString();
			break;
		case rpc_event::RPC_ID_STOP_STREAMING:
			log_custom(0x33ff02) << QString("Action STOP_STREAM (%1)").arg((int)event).toStdString();
			break;

		case rpc_event::RPC_ID_RECORDING_STATUS_CHANGED_SUBSCRIBE:
		case rpc_event::RPC_ID_STREAMING_STATUS_CHANGED_SUBSCRIBE:
		case rpc_event::RPC_ID_COLLECTION_ADDED_SUBSCRIBE:
		case rpc_event::RPC_ID_COLLECTION_REMOVED_SUBSCRIBE:
		case rpc_event::RPC_ID_COLLECTION_UPDATED_SUBSCRIBE:
		case rpc_event::RPC_ID_COLLECTION_SWITCHED_SUBSCRIBE:
			log_custom(0xffb520) << QString("Subscribe Event (%1)").arg((int)event).toStdString();
			break;

		case rpc_event::RPC_ID_FETCH_COLLECTIONS_SCHEMA:
			log_custom(0xffaa9d) << "Fetching collections schema";
			break;

		case rpc_event::RPC_ID_GET_SCENES:
			log_custom(0xebdcd9) << "Get scenes";
			break;

		case rpc_event::RPC_ID_GET_RECORD_STREAM_STATE:
			log_custom(0xed65e6) << "Get Recording and Streaming State";
			break;

		case rpc_event::RPC_ID_GET_ACTIVE_COLLECTION:
			log_custom(0xab83f0) << "Get active collection";

		case rpc_event::RPC_ID_ERROR:
		default:
			log_warn << "Unknown event: ";
			std::string log = json_quest.toJson(QJsonDocument::JsonFormat::Indented).toStdString();
			log_warn << log;
			break;
	}
}

/*
========================================================================================================
	Socket Handling
========================================================================================================
*/

QTcpSocket* StreamdeckClient::socket() const {
	return m_internalSocket;
}

void StreamdeckClient::close() {
	log_custom(LOG_STREAMDECK_CLIENT) << "[Streamdeck Client] Connection was lost with a client.";
	exit(0);
	if(m_internalSocket != nullptr && m_internalSocket->isOpen()) {
		m_internalSocket->close();
		disconnect(m_internalSocket, SIGNAL(disconnected(void)), this, SLOT(disconnected(void)));
		disconnect(m_internalSocket, SIGNAL(readyRead(void)), this, SLOT(read(void)));
		m_internalSocket->deleteLater();
		m_internalSocket = nullptr;
	}
}

void StreamdeckClient::disconnected() {
	emit disconnected(0);
}

/*
========================================================================================================
	Streamdeck Client Management
========================================================================================================
*/

StreamdeckClient* Streamdeck::createClient(qintptr socketDescriptor) {

	bool_s ready = shared_variable<bool>("ready", false);
	bool_s socket_created = shared_variable<bool>("socket_created", false);

	StreamdeckClient* client = new StreamdeckClient(socketDescriptor);
	client->start();

	ready = true;
	socket_created.wait([](const bool& value) {return value == true; });

	if(client->m_internalSocket == nullptr) {
		delete client;
		client = nullptr;
	}

	// While the thread was created in the main thread, it belonged to its.
	client->moveToThread(client);

	return client;
}

void StreamdeckClient::ready() {
	m_startExecution = true;
}

void Streamdeck::close() {
	log_custom(LOG_STREAMDECK) << "[Streamdeck] Closing...";
	emit close_client();
}

void Streamdeck::disconnected(int code) {
	log_warn << "[Streamdeck] A streamdeck lost connection...";
	emit clientDisconnected(this, code);
}