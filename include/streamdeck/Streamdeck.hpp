#pragma once

/*
 * Qt Includes
 */
#include <QTcpSocket>
#include <QObject>
#include <QQueue>
#include <QPair>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QThread>

/*
 * STL Includes
 */
#include <mutex>
#include <thread>
#include <vector>

/*
 * Plugin Includes
 */
#include "include/obs/Collection.hpp"

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class Streamdeck;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class StreamdeckClient : public QThread {

	friend class Streamdeck;

	Q_OBJECT

	/*
	====================================================================================================
		Static Class Attributes
	====================================================================================================
	*/
	private:

		static bool _isVerbose;

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		QTcpSocket* m_internalSocket;

		qintptr m_socketDescriptor;

		bool m_startExecution;

		bool m_isClosing = false;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	private:

		StreamdeckClient(qintptr socjetDescriptor);

		~StreamdeckClient();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		QTcpSocket* socket() const;

		void ready();

	protected:

		void run() override final;

	/*
	====================================================================================================
		Slots
	====================================================================================================
	*/
	private slots:

		void close();

		void disconnected();

		void read();

		void write(QJsonDocument data);

	/*
	====================================================================================================
		Signals
	====================================================================================================
	*/
	signals:

		void disconnected(int code);

	signals:

		void read(QJsonDocument data);

};

class Streamdeck : public QObject {

	Q_OBJECT

	/*
	====================================================================================================
		Types Definitions
	====================================================================================================
	*/
	public:

		enum class rpc_event {
			RPC_ID_ERROR = -1,
			RPC_ID_NO_EVENT = 0,
			RPC_ID_START_STREAMING = 1,
			RPC_ID_STOP_STREAMING = 2,
			RPC_ID_START_RECORDING = 3,
			RPC_ID_STOP_RECORDING = 4,
			RPC_ID_GET_COLLECTIONS = 5,
			RPC_ID_MAKE_COLLECTION_ACTIVE = 6,

			RPC_ID_MISSING_NO = 7,

			RPC_ID_FETCH_COLLECTIONS_SCHEMA = 8,
			RPC_ID_GET_SCENES = 9,
			RPC_ID_GET_SOURCES = 10,
			RPC_ID_MAKE_SCENE_ACTIVE = 11,
			RPC_ID_GET_ACTIVE_SCENE = 12,
			RPC_ID_MUTE_AUDIO_SOURCE = 13,
			RPC_ID_UNMUTE_AUDIO_SOURCE = 14,
			RPC_ID_HIDE_ITEM = 15,
			RPC_ID_SHOW_ITEM = 16,
			RPC_ID_SCENE_SWITCHED_SUBSCRIBE = 17,
			RPC_ID_SCENE_ADDED_SUBSCRIBE = 18,
			RPC_ID_SCENE_REMOVED_SUBSCRIBE = 19,
			RPC_ID_SOURCE_ADDED_SUBSCRIBE = 20,
			RPC_ID_SOURCE_REMOVED_SUBSCRIBE = 21,
			RPC_ID_SOURCE_UPDATED_SUBSCRIBE = 22,
			RPC_ID_ITEM_ADDED_SUBSCRIBED = 23,
			RPC_ID_ITEM_REMOVED_SUBSCRIBE = 24,
			RPC_ID_ITEM_UPDATED_SUBSCRIBE = 25,
			RPC_ID_STREAMING_STATUS_CHANGED_SUBSCRIBE = 26,
			RPC_ID_GET_ACTIVE_COLLECTION = 27,
			RPC_ID_COLLECTION_ADDED_SUBSCRIBE = 28,
			RPC_ID_COLLECTION_REMOVED_SUBSCRIBE = 29,
			RPC_ID_COLLECTION_SWITCHED_SUBSCRIBE = 30,
			RPC_ID_GET_RECORD_STREAM_STATE = 31,
			RPC_ID_COLLECTION_UPDATED_SUBSCRIBE = 32,
			RPC_ID_RECORDING_STATUS_CHANGED_SUBSCRIBE = 33,

			RPC_ID_COUNT,
		};

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	public:

		static StreamdeckClient* createClient(qintptr socketDescriptor);

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::map<rpc_event, std::string> m_subscribedResources;

		std::map<rpc_event, bool> m_authorizedEvents;

		StreamdeckClient& m_internalClient;

	/*
	====================================================================================================
		Constructors / Destructors
	====================================================================================================
	*/
	public:

		Streamdeck(StreamdeckClient& client);
		
		~Streamdeck();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void close();

		void parse(const QJsonDocument& json_quest, Streamdeck::rpc_event& event, QString& service,
			QString& method, QVector<QVariant>& args);

		bool sendSubscriptionMessage(const rpc_event event, const std::string& resourceId);

		bool sendStatusMessage(const rpc_event event, const std::string& status);

		bool sendStatusMessage(const rpc_event event);

		bool sendRecordStreamState(const rpc_event event, const std::string& resourceId,
			const std::string& streaming, const std::string& recording);

		bool sendErrorMessage(const rpc_event event, const std::string& resourceId, bool error);

		bool sendActiveCollectionMessage(const rpc_event event, const std::string& resourceId,
			const Collection* collection);

		bool sendCollectionSwitchMessage(const rpc_event event, const Collection* collection);

		bool sendCollectionsMessage(const rpc_event event, const std::string& resourceId,
			const Collections& collections);

		bool sendCollectionsSchema(const rpc_event event, const Collections& collections);

		bool sendScenesMessage(const rpc_event event, const std::string& resourceId,
			const Collection* collection, const Scenes& scenes);

		void updateEventAuthorizations(const rpc_event event, bool value);

	private:

		QJsonObject buildJsonResponse(const rpc_event event, const QString& resourceId) const;

		QJsonObject buildJsonResult(const rpc_event event, const QString& resourceId) const;

		void addToJsonObject(QJsonObject& json_object, QString key, QJsonValue&& value) const;

		void addToJsonObject(QJsonValueRef&& json_object, QString key, QJsonValue&& value) const;

		void addToJsonArray(QJsonArray& json_array, QJsonValue&& value) const;

		void addToJsonArray(QJsonValueRef&& json_array, QJsonValue&& value) const;

	/*
	====================================================================================================
		Slots
	====================================================================================================
	*/
	public slots:

		void disconnected(int code);
		void read(QJsonDocument data);
	
	/*
	====================================================================================================
		Signals
	====================================================================================================
	*/
	signals:

		void close_client();

	signals:

		void clientDisconnected(Streamdeck*, int);

	signals:

		void received(Streamdeck* client, rpc_event event, QString service, QString method, 
			QVector<QVariant>, bool& error);

	signals:

		void write(QJsonDocument data);

};