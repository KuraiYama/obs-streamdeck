/*
 * CRT Includes
 */
#include <cstdlib>
#include <cmath>

/*
 * Qt Includes
 */
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

/*
 * Plugin Includes
 */
#include "include/common/Global.h"
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

StreamdeckClient::StreamdeckClient(qintptr socketDescriptor) :
	m_socketDescriptor(socketDescriptor),
	m_startExecution(false) {
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

Streamdeck::Streamdeck(StreamdeckClient& client) :
	m_internalClient(client) {
	connect(&m_internalClient, SIGNAL(disconnected(int)), this, SLOT(disconnected(int)));
	connect(&m_internalClient, SIGNAL(read(QJsonDocument)), this, SLOT(read(QJsonDocument)));
	connect(this, SIGNAL(write(QJsonDocument)), &m_internalClient, SLOT(write(QJsonDocument)), 
		Qt::ConnectionType::QueuedConnection);
	connect(this, &Streamdeck::close_client, &m_internalClient, &StreamdeckClient::close);

	memset((byte*)m_authorizedEvents, 0xFF, sizeof(m_authorizedEvents));
}

Streamdeck::~Streamdeck() {

	log_custom(LOG_STREAMDECK) << "[Streamdeck] Destruction...";

	if(m_internalClient.isRunning()) {
		m_internalClient.exit(0);
	}

	disconnect(&m_internalClient, SIGNAL(disconnected(int)), this, SLOT(disconnected(int)));
	disconnect(&m_internalClient, SIGNAL(read(QJsonDocument)), this, SLOT(read(QJsonDocument)));
	disconnect(this, SIGNAL(write(QJsonDocument)), &m_internalClient, SLOT(write(QJsonDocument)));
	disconnect(this, &Streamdeck::close_client, &m_internalClient, &StreamdeckClient::close);

	delete &m_internalClient;
}

/*
========================================================================================================
	Thread Handling
========================================================================================================
*/

void
StreamdeckClient::run() {

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
	exec();
	/****************************/
}

/*
========================================================================================================
	JSON Helpers
========================================================================================================
*/

QJsonObject
Streamdeck::buildJsonResult(const rpc_event event, const QString& resourceId, bool event_mode) {
	QJsonObject response, result;
	response["jsonrpc"] = "2.0";
	response["id"] = (int)event;
	result["resourceId"] = resourceId;
	if(event == rpc_event::NO_EVENT || event_mode)
		result["_type"] = "EVENT";
	response["result"] = result;
	return response;
}

/*QJsonObject
Streamdeck::buildJsonResponse(const rpc_event event, const QString& resourceId) {
	QJsonObject response;
	response["jsonrpc"] = "2.0";
	response["id"] = (int)event;
	response["resourceId"] = resourceId;
	if(event == rpc_event::NO_EVENT)
		response["_type"] = "EVENT";
	return response;
}*/

void
Streamdeck::addToJsonObject(QJsonObject& json_object, QString key, QJsonValue&& value) {
	json_object[key] = value;
}

void
Streamdeck::addToJsonObject(QJsonValueRef&& json_object, QString key, QJsonValue&& value) {
	if(json_object.isObject() == true) {
		QJsonObject copy(json_object.toObject());
		copy[key] = value;
		json_object = copy;
	}
}

void
Streamdeck::addToJsonArray(QJsonArray& json_array, QJsonValue&& value) {
	json_array.append(value);
}

void
Streamdeck::addToJsonArray(QJsonValueRef&& json_array, QJsonValue&& value) {
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

bool
Streamdeck::sendSubscription(const rpc_event event, const std::string& resourceId, bool event_mode) {
	m_subscribedResources[event] = resourceId;
	QJsonObject response = buildJsonResult(event, QString::fromStdString(resourceId), event_mode);

	log_custom(LOG_STREAMDECK) << QString("Subscription to resource : %1.")
		.arg(resourceId.c_str())
		.toStdString();

	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendEvent(const rpc_event event, bool event_mode) {
	if(m_subscribedResources.find(event) == m_subscribedResources.end())
		return false;

	QJsonObject response = buildJsonResult(
		rpc_event::NO_EVENT,
		QString::fromStdString(m_subscribedResources[event]),
		event_mode
	);

	log_custom(LOG_STREAMDECK) << QString("Send Event Message to %1.")
		.arg(m_subscribedResources[event].c_str())
		.toStdString();

	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendRecordStreamState(
	const rpc_event event,
	const std::string& resourceId,
	const std::string& streaming,
	const std::string& recording,
	bool event_mode
) {
	if(event != rpc_event::GET_RECORD_STREAM_STATE)
		return false;

	QJsonObject response = buildJsonResult(
		rpc_event::GET_RECORD_STREAM_STATE,
		QString::fromStdString(resourceId),
		event_mode
	);

	addToJsonObject(response["result"], "streamingStatus", streaming.c_str());
	addToJsonObject(response["result"], "recordingStatus", recording.c_str());

	log_custom(LOG_STREAMDECK) << QString("Send response to event %1").arg((int)event).toStdString();

	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendError(
	const rpc_event event,
	const std::string& resourceId,
	bool error,
	bool event_mode
) {
	QJsonObject response = buildJsonResult(event, QString::fromStdString(resourceId), event_mode);
	this->addToJsonObject(response, "error", error);

	log_custom(LOG_STREAMDECK) << QString("Error Message sent for event %1.")
		.arg((int)event)
		.toStdString();

	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendSchemaMessage(
	const rpc_event event,
	const std::string& resourceId,
	const Collections& collections,
	bool event_mode
) {

	rpc_event ev = event;
	std::string resource = resourceId;

	// In the case of FETCH, Streamdeck doesn't respect its own protocol
	if(event == rpc_event::FETCH_COLLECTIONS_SCHEMA) {
		auto iter = m_subscribedResources.find(rpc_event::FETCH_COLLECTIONS_SCHEMA);
		if(iter == m_subscribedResources.end())
			return false;
		ev = rpc_event::GET_COLLECTIONS;
		resource = iter->second;
		event_mode = true;
	}
#ifdef USE_SCHEMA
	
	QJsonObject response = buildJsonResult(ev, QString::fromStdString(resource));

	if(add_event) addToJsonObject(response["result"], "_type", "EVENT");

	QJsonArray data;
	for(auto iter = collections.begin(); iter < collections.end(); iter++) {
		QJsonObject collection;
		addToJsonObject(collection, "name", (*iter)->name().c_str());
		addToJsonObject(collection, "id", QString("%1").arg((*iter)->id()));
		QJsonArray sources;
		addToJsonObject(collection, "sources", sources);
		// TODO
		addToJsonArray(data, collection);
	}
	addToJsonObject(response["result"], "data", data);

	log_custom(LOG_STREAMDECK) << QString("Send schema.").toStdString();

	send(event, QJsonDocument(response));
	return true;
#else

	return this->sendCollectionsMessage(ev, resource, collections, event_mode);

#endif
}

bool
Streamdeck::sendCollectionsMessage(
	const rpc_event event,
	const std::string& resourceId,
	const Collections& collections,
	bool event_mode
) {
	
	QJsonObject response = buildJsonResult(event, QString::fromStdString(resourceId), event_mode);

	QJsonArray data;
	for(auto iter = collections.begin(); iter < collections.end(); iter++) {
		QJsonObject collection;
		addToJsonObject(collection, "name", (*iter)->name().c_str());
		addToJsonObject(collection, "id", QString("%1").arg((*iter)->id()));
		addToJsonArray(data, collection);
	}
	addToJsonObject(response["result"], "data", data);

	log_custom(LOG_STREAMDECK) << QString("Send collections.").toStdString();

	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendCollectionMessage(
	const rpc_event event,
	const std::string& resourceId,
	const CollectionPtr& collection,
	bool event_mode
) {
	QJsonObject response = buildJsonResult(event, QString::fromStdString(resourceId), event_mode);
	addToJsonObject(response["result"], "id", QString("%1").arg(collection->id()));
	addToJsonObject(response["result"], "name", collection->name().c_str());

	log_custom(LOG_STREAMDECK) << QString("Send collection (Event %1).").arg((int)event).toStdString();
	send(event, QJsonDocument(response));
	return true;
}

/*bool
Streamdeck::sendCollectionSwitchMessage(const rpc_event event, const Collection* collection) {
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
	emit write(QJsonDocument(response));
	updateEventAuthorizations(event, true);
	return true;
}
*/


/*bool
Streamdeck::sendCollectionsSchema(const rpc_event event, const Collections& collections) {

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
	emit this->write(QJsonDocument(response));

	updateEventAuthorizations(rpc_event::RPC_ID_GET_COLLECTIONS, true);

	return true;
}*/

/*bool
Streamdeck::sendScenesMessage(const rpc_event event, const std::string& resourceId,
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

		// We can put it in "items" or "nodes"
		//addToJsonObject(scene_json, "nodes", items_json);
		addToJsonObject(scene_json, "items", items_json);
		addToJsonArray(data, scene_json);
	}
	addToJsonObject(response, "result", data);

	bool result = false;
	emit this->write(QJsonDocument(response));

	updateEventAuthorizations(rpc_event::RPC_ID_GET_COLLECTIONS, true);

	return true;
}*/

/*
========================================================================================================
	Read / Write Operations
========================================================================================================
*/

void
StreamdeckClient::read() {
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

void
StreamdeckClient::write(QJsonDocument document) {
	log_custom(LOG_STREAMDECK_CLIENT) << QString("[Streamdeck Client] Write message...").toStdString();

	if(_isVerbose) {
		std::string log = document.toJson(QJsonDocument::JsonFormat::Indented).toStdString();
		log_custom(LOG_STREAMDECK_CLIENT) << log;
	}

	QByteArray data = document.toJson(QJsonDocument::JsonFormat::Compact).append("\n");
	bool result = false;
	if(m_internalSocket != nullptr && m_internalSocket->isValid()) {
		result = m_internalSocket->write(data) == data.length();
	}

	if(!result)
		m_internalSocket->close();
}

void
Streamdeck::read(QJsonDocument json_quest) {
	if(!m_internalClient.isRunning())
		return;

	log_custom(LOG_STREAMDECK) << "[Streamdeck] Internal client notified a new message.";

	Streamdeck::rpc_event event;
	QString service, method;
	QVector<QVariant> args;

	this->parse(json_quest, event, service, method, args);

	// This event is read-blocked, we skip the event
	if(checkEventAuthorizations(event, EVENT_READ) == false)
		return;
	
	lockEventAuthorizations(event);

	bool error = true;
	emit received(this, event, service, method, args, error);
	if(error)
		close();
}

void
Streamdeck::parse(
	const QJsonDocument& json_quest,
	Streamdeck::rpc_event& event,
	QString& service, 
	QString& method,
	QVector<QVariant>& args
) {
	log_info << "[Streamdeck]Parsing message...";

	event = (rpc_event)(json_quest["id"].isUndefined() ? rpc_event::ERROR : 
		json_quest["id"].toInt() >= (int)rpc_event::COUNT ? rpc_event::ERROR : 
		rpc_event(json_quest["id"].toInt()));

	method = json_quest["method"].toString();

	QJsonObject params = json_quest["params"].toObject();
	service = params["resource"].toString();

	if(params.find("args") != params.end() && params["args"].isArray()) {
		QJsonArray json_args = params["args"].toArray();
		args = QVector<QVariant>::fromList(json_args.toVariantList());
	}

	logEvent(event, json_quest);
}

void
Streamdeck::send(const rpc_event event, const QJsonDocument& json_quest) {

	// This event is read only - skip the message
	if(checkEventAuthorizations(event, EVENT_WRITE) == false)
		return;

	unlockEventAuthorizations(event);

	emit write(json_quest);
}

/*
========================================================================================================
	Authorization Handling
========================================================================================================
*/

bool
Streamdeck::checkEventAuthorizations(const rpc_event event, byte flag) {
	byte index = (((byte)event)-1) / 4;
	byte offset = (((byte)event)-1) % 4;
	byte value = *(m_authorizedEvents + index) >> (2*offset);
	value = value & 0x03;
	return (value & flag) != 0x0;
}

void
Streamdeck::setEventAuthorizations(const rpc_event event, byte flag) {
	byte index = (((byte)event) - 1) / 4;
	byte offset = (((byte)event) - 1) % 4;
	byte value = *(m_authorizedEvents + index) >> (2 * offset);
	value = value & 0x03;
	byte diff = ((value ^ flag) & 0x03) << (2 * offset);
	value = *(m_authorizedEvents + index) ^ diff;
	*(m_authorizedEvents + index) = value;
}

void
Streamdeck::lockEventAuthorizations(const rpc_event event) {
	switch(event) {
		case rpc_event::START_STREAMING:
			setEventAuthorizations(rpc_event::START_STREAMING, EVENT_WRITE);
			setEventAuthorizations(rpc_event::STOP_STREAMING, 0x0);
			setEventAuthorizations(rpc_event::STREAMING_STATUS_CHANGED_SUBSCRIBE, 0x0);
			break;

		case rpc_event::STOP_STREAMING:
			setEventAuthorizations(rpc_event::START_STREAMING, 0x0);
			setEventAuthorizations(rpc_event::STOP_STREAMING, EVENT_WRITE);
			setEventAuthorizations(rpc_event::STREAMING_STATUS_CHANGED_SUBSCRIBE, 0x0);
			break;

		case rpc_event::START_RECORDING:
			setEventAuthorizations(rpc_event::START_RECORDING, EVENT_WRITE);
			setEventAuthorizations(rpc_event::STOP_RECORDING, 0x0);
			setEventAuthorizations(rpc_event::RECORDING_STATUS_CHANGED_SUBSCRIBE, 0x0);
			break;

		case rpc_event::STOP_RECORDING:
			setEventAuthorizations(rpc_event::START_RECORDING, 0x0);
			setEventAuthorizations(rpc_event::STOP_RECORDING, EVENT_WRITE);
			setEventAuthorizations(rpc_event::RECORDING_STATUS_CHANGED_SUBSCRIBE, 0x0);
			break;

		case rpc_event::FETCH_COLLECTIONS_SCHEMA:
		case rpc_event::GET_COLLECTIONS:
			setEventAuthorizations(rpc_event::FETCH_COLLECTIONS_SCHEMA, EVENT_WRITE);
			setEventAuthorizations(rpc_event::GET_COLLECTIONS, EVENT_WRITE);
			setEventAuthorizations(rpc_event::COLLECTION_ADDED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc_event::COLLECTION_REMOVED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc_event::COLLECTION_UPDATED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc_event::COLLECTION_SWITCHED_SUBSCRIBE, 0x0);
			break;

		default:
			break;
	}
}

void
Streamdeck::unlockEventAuthorizations(const rpc_event event) {
	switch(event) {
		case rpc_event::START_STREAMING:
			setEventAuthorizations(rpc_event::START_STREAMING, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::STOP_STREAMING, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::STREAMING_STATUS_CHANGED_SUBSCRIBE, EVENT_READ_WRITE);
			break;

		case rpc_event::STOP_STREAMING:
			setEventAuthorizations(rpc_event::START_STREAMING, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::STOP_STREAMING, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::STREAMING_STATUS_CHANGED_SUBSCRIBE, EVENT_READ_WRITE);
			break;

		case rpc_event::START_RECORDING:
			setEventAuthorizations(rpc_event::START_RECORDING, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::STOP_RECORDING, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::RECORDING_STATUS_CHANGED_SUBSCRIBE, EVENT_READ_WRITE);
			break;

		case rpc_event::STOP_RECORDING:
			setEventAuthorizations(rpc_event::START_RECORDING, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::STOP_RECORDING, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::RECORDING_STATUS_CHANGED_SUBSCRIBE, EVENT_READ_WRITE);
			break;

		case rpc_event::GET_COLLECTIONS:
			setEventAuthorizations(rpc_event::FETCH_COLLECTIONS_SCHEMA, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::GET_COLLECTIONS, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::COLLECTION_ADDED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::COLLECTION_REMOVED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::COLLECTION_UPDATED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::FETCH_COLLECTIONS_SCHEMA, EVENT_READ_WRITE);
			setEventAuthorizations(rpc_event::COLLECTION_SWITCHED_SUBSCRIBE, EVENT_READ_WRITE);
			break;

		default:
			break;
	}
}

/*
========================================================================================================
	Socket Handling
========================================================================================================
*/

QTcpSocket*
StreamdeckClient::socket() const {
	return m_internalSocket;
}

void
StreamdeckClient::close() {
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

void
StreamdeckClient::disconnected() {
	emit disconnected(0);
}

/*
========================================================================================================
	Streamdeck Client Management
========================================================================================================
*/

StreamdeckClient*
Streamdeck::createClient(qintptr socketDescriptor) {

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

void
StreamdeckClient::ready() {
	m_startExecution = true;
}

void
Streamdeck::close() {
	log_custom(LOG_STREAMDECK) << "[Streamdeck] Closing...";
	emit close_client();
}

void
Streamdeck::disconnected(int code) {
	log_warn << "[Streamdeck] A streamdeck lost connection...";
	emit clientDisconnected(this, code);
}

/*
========================================================================================================
	Logging
========================================================================================================
*/

void
Streamdeck::logEvent(const rpc_event event, const QJsonDocument& json_quest) {
	switch(event) {
		case rpc_event::START_RECORDING:
			log_custom(0x33ff02) << QString("Action START_RECORD (%1)").arg((int)event).toStdString();
			break;
		case rpc_event::STOP_RECORDING:
			log_custom(0x33ff02) << QString("Action STOP_RECORD (%1)").arg((int)event).toStdString();
			break;
		case rpc_event::START_STREAMING:
			log_custom(0x33ff02) << QString("Action START_STREAM (%1)").arg((int)event).toStdString();
			break;
		case rpc_event::STOP_STREAMING:
			log_custom(0x33ff02) << QString("Action STOP_STREAM (%1)").arg((int)event).toStdString();
			break;

		case rpc_event::RECORDING_STATUS_CHANGED_SUBSCRIBE:
		case rpc_event::STREAMING_STATUS_CHANGED_SUBSCRIBE:
		case rpc_event::COLLECTION_ADDED_SUBSCRIBE:
		case rpc_event::COLLECTION_REMOVED_SUBSCRIBE:
		case rpc_event::COLLECTION_UPDATED_SUBSCRIBE:
		/*case rpc_event::RPC_ID_COLLECTION_SWITCHED_SUBSCRIBE:*/
			log_custom(0xffb520) << QString("Subscribe Event (%1)").arg((int)event).toStdString();
			break;

		case rpc_event::GET_COLLECTIONS:
			log_custom(0x89dcff) << "Get Collections List";
			break;

		case rpc_event::FETCH_COLLECTIONS_SCHEMA:
			log_custom(0xffaa9d) << "Fetching collections schema";
			break;

		/*case rpc_event::RPC_ID_GET_SCENES:
			log_custom(0xebdcd9) << "Get scenes";
			break;*/

		case rpc_event::GET_RECORD_STREAM_STATE:
			log_custom(0xed65e6) << "Get Recording and Streaming State";
			break;

		/*case rpc_event::RPC_ID_GET_ACTIVE_COLLECTION:
			log_custom(0xab83f0) << "Get active collection";*/

		case rpc_event::ERROR:
		default:
			log_warn << "Unknown event: ";
			std::string log = json_quest.toJson(QJsonDocument::JsonFormat::Indented).toStdString();
			log_warn << log;
			break;
	}
}