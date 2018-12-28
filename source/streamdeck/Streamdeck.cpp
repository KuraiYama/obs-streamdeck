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
#include "include/Global.h"
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

bool StreamdeckClient::_is_verbose = false;

/*
========================================================================================================
	Constructor / Destructors
========================================================================================================
*/

StreamdeckClient::StreamdeckClient(qintptr socket_descriptor) :
	m_socketDescriptor(socket_descriptor),
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
	log_custom(LOG_STREAMDECK_CLIENT) << "[Streamdeck Client] Destroyed." << log_end;
}

Streamdeck::Streamdeck(StreamdeckClient& client) :
	m_internalClient(client) {
	connect(&m_internalClient, SIGNAL(disconnected(int)), this, SLOT(disconnected(int)));
	connect(&m_internalClient, SIGNAL(read(QJsonDocument)), this, SLOT(read(QJsonDocument)));
	connect(this, SIGNAL(write(QJsonDocument)), &m_internalClient, SLOT(write(QJsonDocument)), 
		Qt::ConnectionType::QueuedConnection);
	connect(this, &Streamdeck::close_client, &m_internalClient, &StreamdeckClient::close);

	memset((byte*)m_authorizedEvents, 0xFF, sizeof(m_authorizedEvents));

	setEventAuthorizations(rpc::event::START_STREAMING, EVENT_READ);
	setEventAuthorizations(rpc::event::STOP_STREAMING, EVENT_READ);
	setEventAuthorizations(rpc::event::START_RECORDING, EVENT_READ);
	setEventAuthorizations(rpc::event::STOP_RECORDING, EVENT_READ);
	setEventAuthorizations(rpc::event::MAKE_SCENE_ACTIVE, EVENT_READ);
}

Streamdeck::~Streamdeck() {

	log_custom(LOG_STREAMDECK) << "[Streamdeck] Destruction..." << log_end;

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

	log_custom(LOG_STREAMDECK_CLIENT) << "[Streamdeck Client] New thread run." << log_end;

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

	log_custom(LOG_STREAMDECK_CLIENT) << "[Streamdeck Client] Socket created." << log_end;

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
Streamdeck::buildJsonResult(const rpc::event event, const QString& resource, bool event_mode) {
	QJsonObject response, result;
	response["jsonrpc"] = "2.0";
	response["id"] = (int)event;
	result["resourceId"] = resource;
	if(event == rpc::event::NO_EVENT || event_mode)
		result["_type"] = "EVENT";
	response["result"] = result;
	return response;
}

QJsonObject
Streamdeck::buildJsonResponse(const rpc::event event, const QString& resource, bool event_mode) {
	QJsonObject response;
	response["jsonrpc"] = "2.0";
	response["id"] = (int)event;
	response["resourceId"] = resource;
	if(event == rpc::event::NO_EVENT || event_mode)
		response["_type"] = "EVENT";
	return response;
}

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
Streamdeck::sendAcknowledge(const rpc::event event, const std::string& resource, bool event_mode) {
	QJsonObject response = buildJsonResponse(event, QString::fromStdString(resource), event_mode);

	log_custom(LOG_STREAMDECK) << QString("Acknowledge event %1 (%2).")
		.arg(QString("%1").arg((int)event))
		.arg(resource.c_str())
		.toStdString() << log_end;

	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendSubscription(const rpc::event event, const std::string& resource, bool event_mode) {
	m_subscribedResources[event] = resource;
	QJsonObject response = buildJsonResult(event, QString::fromStdString(resource), event_mode);

	log_custom(LOG_STREAMDECK) << QString("Subscription to resource : %1.")
		.arg(resource.c_str())
		.toStdString() << log_end;

	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendEvent(const rpc::event event, bool event_mode) {
	if(m_subscribedResources.find(event) == m_subscribedResources.end())
		return false;

	QJsonObject response = buildJsonResult(
		rpc::event::NO_EVENT,
		QString::fromStdString(m_subscribedResources[event]),
		event_mode
	);

	log_custom(LOG_STREAMDECK) << QString("Send Event Message to %1.")
		.arg(m_subscribedResources[event].c_str())
		.toStdString() << log_end;

	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendRecordStreamState(
	const rpc::event event,
	const std::string& resource,
	const std::string& streaming,
	const std::string& recording,
	bool event_mode
) {
	if(event != rpc::event::GET_RECORD_STREAM_STATE)
		return false;

	QJsonObject response = buildJsonResult(
		rpc::event::GET_RECORD_STREAM_STATE,
		QString::fromStdString(resource),
		event_mode
	);

	addToJsonObject(response["result"], "streamingStatus", streaming.c_str());
	addToJsonObject(response["result"], "recordingStatus", recording.c_str());

	log_custom(LOG_STREAMDECK) << QString("Send response to event %1").arg((int)event).toStdString()
		<< log_end;

	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendError(
	const rpc::event event,
	const std::string& resource,
	bool error,
	bool event_mode
) {
	QJsonObject response = buildJsonResult(event, QString::fromStdString(resource), event_mode);
	this->addToJsonObject(response, "error", error);

	log_custom(LOG_STREAMDECK) << QString("Error Message sent for event %1.")
		.arg((int)event)
		.toStdString() << log_end;

	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendSchema(
	const rpc::event event,
	const std::string& resource,
	const Collections& collections,
	bool event_mode
) {

	rpc::event ev = event;
	std::string res = resource;

	// In the case of FETCH, Streamdeck doesn't respect its own protocol
	if(event == rpc::event::FETCH_COLLECTIONS_SCHEMA) {
		auto iter = m_subscribedResources.find(rpc::event::FETCH_COLLECTIONS_SCHEMA);
		if(iter == m_subscribedResources.end())
			return false;
		ev = rpc::event::GET_COLLECTIONS;
		res = iter->second;
		event_mode = true;
	}
#ifdef USE_SCHEMA
	
	QJsonObject response = buildJsonResult(ev, QString::fromStdString(res), event_mode);

	QJsonArray data;
	for(auto iter = collections.begin(); iter < collections.end(); iter++) {
		QJsonObject collection;
		addToJsonObject(collection, "name", (*iter)->name().c_str());
		uint16_t collection_id = (*iter)->id();
		addToJsonObject(collection, "id", QString("%1").arg(collection_id));
		QJsonArray scenes_json;
		Scenes scenes = (*iter)->scenes();
		for(auto iter_sc = scenes.scenes.begin(); iter_sc < scenes.scenes.end(); iter_sc++) {
			QJsonObject scene;
			uint64_t scene_id = (collection_id << 17) + (*iter_sc)->id();
			addToJsonObject(scene, "id", QString("%1").arg(scene_id));
			addToJsonObject(scene, "name", QString::fromStdString((*iter_sc)->name()));

			QJsonArray items_json;
			Items items = (*iter_sc)->items();
			for(auto iter_it = items.items.begin(); iter_it < items.items.end(); iter_it++) {
				QJsonObject item;
				uint64_t item_id = (scene_id << 24) + /* type << 8 */ + (*iter_it)->id();
				addToJsonObject(item, "sceneItemId", QString("%1").arg(item_id));
				const Source* sourceRef = (*iter_it)->source();
				uint64_t source_id =
					(sourceRef->collection()->id() << 17) + (1 << 16) + sourceRef->id();
				addToJsonObject(item, "sourceId", QString("%1").arg(source_id));
				addToJsonObject(item, "visible", (*iter_it)->visible());
				addToJsonArray(items_json, item);
			}
			addToJsonObject(scene, "sceneItems", items_json);
			addToJsonArray(scenes_json, scene);
		}
		addToJsonObject(collection, "scenes", scenes_json);

		QJsonArray sources_json;
		Sources sources = (*iter)->sources();
		for(auto iter_src = sources.sources.begin(); iter_src != sources.sources.end(); iter_src++) {
			QJsonObject source;
			uint64_t source_id = (collection_id << 17) + (1 << 16) + (*iter_src)->id();
			addToJsonObject(scene, "id", QString("%1").arg(source_id));
			addToJsonObject(scene, "name", QString::fromStdString((*iter_src)->name()));
			addToJsonObject(source, "type", (*iter_src)->type());
			addToJsonObject(source, "muted", (*iter_src)->muted());
			addToJsonObject(source, "audio", (*iter_src)->audio());
			addToJsonArray(sources_json, source);
		}
		addToJsonObject(collection, "sources", sources_json);

		addToJsonArray(data, collection);
	}
	addToJsonObject(response["result"], "data", data);

	log_custom(LOG_STREAMDECK) << QString("Send schema.").toStdString();

	send(ev, QJsonDocument(response));
	return true;
#else

	return this->sendCollections(ev, res, collections, event_mode);

#endif
}

bool
Streamdeck::sendCollections(
	const rpc::event event,
	const std::string& resource,
	const Collections& collections,
	bool event_mode
) {
	QJsonObject response = buildJsonResult(event, QString::fromStdString(resource), event_mode);

	QJsonArray data;
	for(auto iter = collections.begin(); iter < collections.end(); iter++) {
		QJsonObject collection;
		addToJsonObject(collection, "name", (*iter)->name().c_str());
		addToJsonObject(collection, "id", QString("%1").arg((*iter)->id()));
		addToJsonArray(data, collection);
	}
	addToJsonObject(response["result"], "data", data);

	log_custom(LOG_STREAMDECK) << QString("Send collections.").toStdString() << log_end;

	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendCollection(
	const rpc::event event,
	const std::string& resource,
	const CollectionPtr& collection,
	bool event_mode
) {
	QJsonObject response = buildJsonResult(event, QString::fromStdString(resource), event_mode);
	addToJsonObject(response["result"], "id", QString("%1").arg(collection->id()));
	addToJsonObject(response["result"], "name", collection->name().c_str());

	log_custom(LOG_STREAMDECK) << QString("Send collection (Event %1).").arg((int)event).toStdString()
		<< log_end;
	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendScenes(
	const rpc::event event,
	const std::string& resource,
	const Scenes& scenes,
	bool event_mode
) {
	QJsonObject response = buildJsonResponse(event, QString::fromStdString(resource), event_mode);
	QString collection_id = "";
	if(scenes.collection != nullptr)
		collection_id = QString("%1").arg(scenes.collection->id());
	addToJsonObject(response, "collection", collection_id);

	QJsonArray result;
	for(auto iter_sc = scenes.scenes.begin(); iter_sc < scenes.scenes.end(); iter_sc++) {
		Scene* scene_ptr = (*iter_sc);
		QJsonObject scene;
		uint64_t scene_id = scene_ptr->id();
		if(scenes.collection != nullptr)
			scene_id += (scenes.collection->id() << 17);
		addToJsonObject(scene, "id", QString("%1").arg(scene_id));
		addToJsonObject(scene, "name", QString::fromStdString(scene_ptr->name()));

		QJsonArray nodes_items;
		std::vector<ItemPtr> items = std::move(scene_ptr->items().items);
		for(auto iter_it = items.begin(); iter_it < items.end(); iter_it++) {
			Item* item_ptr = (*iter_it);
			QJsonObject item;
			uint64_t item_id = (scene_id << 24) + /* type << 8 */ + item_ptr->id();
			addToJsonObject(item, "sceneItemId", QString("%1").arg(item_id));
			const Source* sourceRef = item_ptr->source();
			uint64_t source_id = (sourceRef->collection()->id() << 17) + (1 << 16) + sourceRef->id();
			addToJsonObject(item, "sourceId", QString("%1").arg(source_id));
			addToJsonObject(item, "visible", item_ptr->visible());
			addToJsonObject(item, "sceneNodeType", item_ptr->type());

			addToJsonArray(nodes_items, item);
		}
#ifndef NO_SEND_ITEMS
		addToJsonObject(scene, ((std::rand() % 2) == 0 ? "nodes" : "items"), nodes_items);
#endif
		addToJsonArray(result, scene);
	}
	addToJsonObject(response, "result", result);

	log_custom(LOG_STREAMDECK) << QString("Send scenes.").toStdString() << log_end;

	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendScene(
	const rpc::event event,
	const std::string& resource,
	const ScenePtr& scene,
	bool event_mode
) {
	QJsonObject response = buildJsonResponse(event, QString::fromStdString(resource), event_mode);
	uint64_t id = (scene->collection()->id() << 17) + scene->id();
	addToJsonObject(response, "result", QString("%1").arg(id));
	addToJsonObject(response, "collection", QString("%1").arg(scene->collection()->id()));

	log_custom(LOG_STREAMDECK) << QString("Send scene (Event %1).").arg((int)event).toStdString()
		<< log_end;
	send(event, QJsonDocument(response));
	return true;
}

bool
Streamdeck::sendSources(
	const rpc::event event,
	const std::string& resource,
	const Sources& sources,
	bool event_mode
) {
	QJsonObject response = buildJsonResponse(event, QString::fromStdString(resource), event_mode);
	QString collection_id = "";
	if(sources.collection != nullptr)
		collection_id = QString("%1").arg(sources.collection->id());
	addToJsonObject(response, "collection", collection_id);

	QJsonArray result;
	for(auto iter_src = sources.sources.begin(); iter_src < sources.sources.end(); iter_src++) {
		Source* source_ptr = (*iter_src);
		QJsonObject source;
		uint64_t source_id = source_ptr->id();
		if(sources.collection != nullptr)
			source_id += (sources.collection->id() << 17) + (1 << 16);
		addToJsonObject(source, "id", QString("%1").arg(source_id));
		addToJsonObject(source, "name", QString::fromStdString(source_ptr->name()));
		addToJsonObject(source, "type", source_ptr->type());
		addToJsonObject(source, "muted", source_ptr->muted());
		addToJsonObject(source, "audio", source_ptr->audio());
		addToJsonArray(result, source);
	}
	addToJsonObject(response, "result", result);

	log_custom(LOG_STREAMDECK) << QString("Send sources.").toStdString() << log_end;

	send(event, QJsonDocument(response));
	return true;
}

/*
========================================================================================================
	Read / Write Operations
========================================================================================================
*/

void
StreamdeckClient::read() {
	while(m_internalSocket != nullptr && m_internalSocket->canReadLine()) {
		try {
			log_custom(LOG_STREAMDECK_CLIENT) << "[Streamdeck Client] Read message..." << log_end;
			QByteArray data = m_internalSocket->readLine();
			QJsonDocument json_quest = QJsonDocument::fromJson(data);
			if(_is_verbose) {
				std::string log = json_quest.toJson(QJsonDocument::JsonFormat::Indented).toStdString();
				log_custom(LOG_STREAMDECK_CLIENT) << log << log_end;
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
	log_custom(LOG_STREAMDECK_CLIENT) << QString("[Streamdeck Client] Write message...").toStdString()
		<< log_end;

	if(_is_verbose) {
		std::string log = document.toJson(QJsonDocument::JsonFormat::Indented).toStdString();
		log_custom(LOG_STREAMDECK_CLIENT) << log << log_end;
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

	log_custom(LOG_STREAMDECK) << "[Streamdeck] Internal client notified a new message." << log_end;

	rpc::event event;
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
	rpc::event& event,
	QString& service, 
	QString& method,
	QVector<QVariant>& args
) {
	log_info << "[Streamdeck]Parsing message..." << log_end;

	event = (rpc::event)(json_quest["id"].isUndefined() ? rpc::event::ERROR :
		json_quest["id"].toInt() >= (int)rpc::event::COUNT ? rpc::event::ERROR :
		rpc::event(json_quest["id"].toInt()));

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
Streamdeck::send(const rpc::event event, const QJsonDocument& json_quest) {

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
Streamdeck::checkEventAuthorizations(const rpc::event event, byte flag) {
	byte index = (((byte)event)-1) / 4;
	byte offset = (((byte)event)-1) % 4;
	byte value = *(m_authorizedEvents + index) >> (2*offset);
	value = value & 0x03;
	return (value & flag) != 0x0;
}

void
Streamdeck::setEventAuthorizations(const rpc::event event, byte flag) {
	byte index = (((byte)event) - 1) / 4;
	byte offset = (((byte)event) - 1) % 4;
	byte value = *(m_authorizedEvents + index) >> (2 * offset);
	value = value & 0x03;
	byte diff = ((value ^ flag) & 0x03) << (2 * offset);
	value = *(m_authorizedEvents + index) ^ diff;
	*(m_authorizedEvents + index) = value;
}

void
Streamdeck::lockEventAuthorizations(const rpc::event event) {
	switch(event) {
		case rpc::event::START_STREAMING:
			setEventAuthorizations(rpc::event::START_STREAMING, EVENT_WRITE);
			setEventAuthorizations(rpc::event::STOP_STREAMING, 0x0);
			setEventAuthorizations(rpc::event::STREAMING_STATUS_CHANGED_SUBSCRIBE, 0x0);
			break;

		case rpc::event::STOP_STREAMING:
			setEventAuthorizations(rpc::event::START_STREAMING, 0x0);
			setEventAuthorizations(rpc::event::STOP_STREAMING, EVENT_WRITE);
			setEventAuthorizations(rpc::event::STREAMING_STATUS_CHANGED_SUBSCRIBE, 0x0);
			break;

		case rpc::event::START_RECORDING:
			setEventAuthorizations(rpc::event::START_RECORDING, EVENT_WRITE);
			setEventAuthorizations(rpc::event::STOP_RECORDING, 0x0);
			setEventAuthorizations(rpc::event::RECORDING_STATUS_CHANGED_SUBSCRIBE, 0x0);
			break;

		case rpc::event::STOP_RECORDING:
			setEventAuthorizations(rpc::event::START_RECORDING, 0x0);
			setEventAuthorizations(rpc::event::STOP_RECORDING, EVENT_WRITE);
			setEventAuthorizations(rpc::event::RECORDING_STATUS_CHANGED_SUBSCRIBE, 0x0);
			break;

		case rpc::event::MAKE_SCENE_ACTIVE:
			setEventAuthorizations(rpc::event::MAKE_COLLECTION_ACTIVE, 0x0);
			setEventAuthorizations(rpc::event::MAKE_SCENE_ACTIVE, EVENT_WRITE);
			setEventAuthorizations(rpc::event::FETCH_COLLECTIONS_SCHEMA, 0x0);
			setEventAuthorizations(rpc::event::GET_COLLECTIONS, 0x0);
			setEventAuthorizations(rpc::event::GET_SCENES, 0x0);
			setEventAuthorizations(rpc::event::COLLECTION_ADDED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::COLLECTION_REMOVED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::COLLECTION_UPDATED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::COLLECTION_SWITCHED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::GET_ACTIVE_COLLECTION, 0x0);
			setEventAuthorizations(rpc::event::GET_ACTIVE_SCENE, 0x0);
			setEventAuthorizations(rpc::event::SCENE_ADDED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SCENE_REMOVED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SCENE_SWITCHED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::ITEM_ADDED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::ITEM_REMOVED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::ITEM_UPDATED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SOURCE_ADDED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SOURCE_REMOVED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SOURCE_UPDATED_SUBSCRIBE, 0x0);
			break;

		case rpc::event::MAKE_COLLECTION_ACTIVE:
			setEventAuthorizations(rpc::event::MAKE_COLLECTION_ACTIVE, EVENT_WRITE);
			setEventAuthorizations(rpc::event::MAKE_SCENE_ACTIVE, 0x0);
			setEventAuthorizations(rpc::event::FETCH_COLLECTIONS_SCHEMA, 0x0);
			setEventAuthorizations(rpc::event::GET_COLLECTIONS, 0x0);
			setEventAuthorizations(rpc::event::GET_SCENES, 0x0);
			setEventAuthorizations(rpc::event::COLLECTION_ADDED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::COLLECTION_REMOVED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::COLLECTION_UPDATED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::COLLECTION_SWITCHED_SUBSCRIBE, EVENT_WRITE);
			setEventAuthorizations(rpc::event::GET_ACTIVE_COLLECTION, 0x0);
			setEventAuthorizations(rpc::event::GET_ACTIVE_SCENE, 0x0);
			setEventAuthorizations(rpc::event::SCENE_ADDED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SCENE_REMOVED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SCENE_SWITCHED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::ITEM_ADDED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::ITEM_REMOVED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::ITEM_UPDATED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SOURCE_ADDED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SOURCE_REMOVED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SOURCE_UPDATED_SUBSCRIBE, 0x0);
			break;

		case rpc::event::FETCH_COLLECTIONS_SCHEMA:
		case rpc::event::GET_COLLECTIONS:
		case rpc::event::GET_SCENES:
		case rpc::event::GET_SOURCES:
			setEventAuthorizations(rpc::event::FETCH_COLLECTIONS_SCHEMA, EVENT_WRITE);
			setEventAuthorizations(rpc::event::GET_COLLECTIONS, EVENT_WRITE);
			setEventAuthorizations(rpc::event::GET_SCENES, EVENT_WRITE);
			setEventAuthorizations(rpc::event::GET_SOURCES, EVENT_WRITE);
			setEventAuthorizations(rpc::event::COLLECTION_ADDED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::COLLECTION_REMOVED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::COLLECTION_UPDATED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::COLLECTION_SWITCHED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::GET_ACTIVE_COLLECTION, 0x0);
			setEventAuthorizations(rpc::event::GET_ACTIVE_SCENE, 0x0);
			setEventAuthorizations(rpc::event::SCENE_ADDED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SCENE_REMOVED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SCENE_SWITCHED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::ITEM_ADDED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::ITEM_REMOVED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::ITEM_UPDATED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SOURCE_ADDED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SOURCE_REMOVED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::SOURCE_UPDATED_SUBSCRIBE, 0x0);
			setEventAuthorizations(rpc::event::MAKE_COLLECTION_ACTIVE, 0x0);
			break;

		default:
			break;
	}
}

void
Streamdeck::unlockEventAuthorizations(const rpc::event event) {
	switch(event) {
		case rpc::event::START_STREAMING:
			setEventAuthorizations(rpc::event::START_STREAMING, EVENT_READ);
			setEventAuthorizations(rpc::event::STOP_STREAMING, EVENT_READ);
			setEventAuthorizations(rpc::event::STREAMING_STATUS_CHANGED_SUBSCRIBE, EVENT_READ_WRITE);
			break;

		case rpc::event::STOP_STREAMING:
			setEventAuthorizations(rpc::event::START_STREAMING, EVENT_READ);
			setEventAuthorizations(rpc::event::STOP_STREAMING, EVENT_READ);
			setEventAuthorizations(rpc::event::STREAMING_STATUS_CHANGED_SUBSCRIBE, EVENT_READ_WRITE);
			break;

		case rpc::event::START_RECORDING:
			setEventAuthorizations(rpc::event::START_RECORDING, EVENT_READ);
			setEventAuthorizations(rpc::event::STOP_RECORDING, EVENT_READ);
			setEventAuthorizations(rpc::event::RECORDING_STATUS_CHANGED_SUBSCRIBE, EVENT_READ_WRITE);
			break;

		case rpc::event::STOP_RECORDING:
			setEventAuthorizations(rpc::event::START_RECORDING, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::STOP_RECORDING, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::RECORDING_STATUS_CHANGED_SUBSCRIBE, EVENT_READ_WRITE);
			break;

		case rpc::event::COLLECTION_SWITCHED_SUBSCRIBE:
			setEventAuthorizations(rpc::event::MAKE_SCENE_ACTIVE, EVENT_READ);
			break;

		case rpc::event::MAKE_SCENE_ACTIVE:
		case rpc::event::MAKE_COLLECTION_ACTIVE:
			setEventAuthorizations(rpc::event::MAKE_SCENE_ACTIVE, EVENT_READ);
		case rpc::event::GET_COLLECTIONS:
		case rpc::event::GET_SCENES:
		case rpc::event::GET_SOURCES:
			setEventAuthorizations(rpc::event::MAKE_COLLECTION_ACTIVE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::FETCH_COLLECTIONS_SCHEMA, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::GET_COLLECTIONS, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::GET_SCENES, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::GET_SOURCES, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::COLLECTION_ADDED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::COLLECTION_REMOVED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::COLLECTION_UPDATED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::COLLECTION_SWITCHED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::GET_ACTIVE_COLLECTION, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::GET_ACTIVE_SCENE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::SCENE_ADDED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::SCENE_REMOVED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::SCENE_SWITCHED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::ITEM_ADDED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::ITEM_REMOVED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::ITEM_UPDATED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::SOURCE_ADDED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::SOURCE_REMOVED_SUBSCRIBE, EVENT_READ_WRITE);
			setEventAuthorizations(rpc::event::SOURCE_UPDATED_SUBSCRIBE, EVENT_READ_WRITE);
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
	log_custom(LOG_STREAMDECK_CLIENT) << "[Streamdeck Client] Connection was lost with a client."
		<< log_end;
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
Streamdeck::createClient(qintptr socket_descriptor) {

	bool_s ready = shared_variable<bool>("ready", false);
	bool_s socket_created = shared_variable<bool>("socket_created", false);

	StreamdeckClient* client = new StreamdeckClient(socket_descriptor);
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
	log_custom(LOG_STREAMDECK) << "[Streamdeck] Closing..." << log_end;
	emit close_client();
}

void
Streamdeck::disconnected(int code) {
	log_warn << "[Streamdeck] A streamdeck lost connection..." << log_end;
	emit clientDisconnected(this, code);
}

/*
========================================================================================================
	Logging
========================================================================================================
*/

void
Streamdeck::logEvent(const rpc::event event, const QJsonDocument& json_quest) {
	switch(event) {
		case rpc::event::START_RECORDING:
			log_custom(0x33ff02) << QString("Action START_RECORD (%1)").arg((int)event).toStdString()
				<< log_end;
			break;
		case rpc::event::STOP_RECORDING:
			log_custom(0x33ff02) << QString("Action STOP_RECORD (%1)").arg((int)event).toStdString()
				<< log_end;
			break;
		case rpc::event::START_STREAMING:
			log_custom(0x33ff02) << QString("Action START_STREAM (%1)").arg((int)event).toStdString()
				<< log_end;
			break;
		case rpc::event::STOP_STREAMING:
			log_custom(0x33ff02) << QString("Action STOP_STREAM (%1)").arg((int)event).toStdString()
				<< log_end;
			break;

		case rpc::event::RECORDING_STATUS_CHANGED_SUBSCRIBE:
		case rpc::event::STREAMING_STATUS_CHANGED_SUBSCRIBE:
		case rpc::event::COLLECTION_ADDED_SUBSCRIBE:
		case rpc::event::COLLECTION_REMOVED_SUBSCRIBE:
		case rpc::event::COLLECTION_UPDATED_SUBSCRIBE:
		case rpc::event::COLLECTION_SWITCHED_SUBSCRIBE:
		case rpc::event::SCENE_ADDED_SUBSCRIBE:
		case rpc::event::SCENE_REMOVED_SUBSCRIBE:
		case rpc::event::SCENE_SWITCHED_SUBSCRIBE:
		case rpc::event::ITEM_ADDED_SUBSCRIBE:
		case rpc::event::ITEM_REMOVED_SUBSCRIBE:
		case rpc::event::ITEM_UPDATED_SUBSCRIBE:
		case rpc::event::SOURCE_ADDED_SUBSCRIBE:
		case rpc::event::SOURCE_REMOVED_SUBSCRIBE:
		case rpc::event::SOURCE_UPDATED_SUBSCRIBE:
			log_custom(0xffb520) << QString("Subscribe Event (%1)").arg((int)event).toStdString()
				<< log_end;
			break;

		case rpc::event::GET_COLLECTIONS:
			log_custom(0x89dcff) << "Get Collections List" << log_end;
			break;

		case rpc::event::FETCH_COLLECTIONS_SCHEMA:
			log_custom(0xffaa9d) << "Fetching collections schema" << log_end;
			break;

		case rpc::event::GET_SCENES:
			log_custom(0xebdcd9) << "Get scenes" << log_end;
			break;

		case rpc::event::GET_SOURCES:
			log_custom(0x04ff86) << "Get sources" << log_end;
			break;

		case rpc::event::GET_RECORD_STREAM_STATE:
			log_custom(0xed65e6) << "Get Recording and Streaming State" << log_end;
			break;

		case rpc::event::GET_ACTIVE_COLLECTION:
			log_custom(0xab83f0) << "Get active collection" << log_end;
			break;

		case rpc::event::GET_ACTIVE_SCENE:
			log_custom(0xffe749) << "Get active scene" << log_end;
			break;

		case rpc::event::MAKE_COLLECTION_ACTIVE:
			log_custom(0xffada4) << "Make collection active" << log_end;
			break;

		case rpc::event::MAKE_SCENE_ACTIVE:
			log_custom(0x13abb0) << "Make scene active" << log_end;
			break;

		case rpc::event::ERROR:
		default:
			log_warn << "Unknown event: " << log_end;
			std::string log = json_quest.toJson(QJsonDocument::JsonFormat::Indented).toStdString();
			log_warn << log << log_end;
			break;
	}
}