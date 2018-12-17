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

typedef unsigned char byte;

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

		volatile bool m_startExecution;

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

		QTcpSocket*
		socket() const;

		void
		ready();

	protected:

		void
		run() override final;

	/*
	====================================================================================================
		Slots
	====================================================================================================
	*/
	private slots:

		void
		close();

		void
		disconnected();

		void
		read();

		void
		write(QJsonDocument data);

	/*
	====================================================================================================
		Signals
	====================================================================================================
	*/
	signals:

		void
		disconnected(int code);

	signals:

		void
		read(QJsonDocument data);

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
			ERROR = -1,
			NO_EVENT = 0,
			START_STREAMING = 1,
			STOP_STREAMING = 2,
			START_RECORDING = 3,
			STOP_RECORDING = 4,
			GET_COLLECTIONS = 5,
			MAKE_COLLECTION_ACTIVE = 6,

			MISSING_NO = 7,

			FETCH_COLLECTIONS_SCHEMA = 8,
			GET_SCENES = 9,
			GET_SOURCES = 10,
			MAKE_SCENE_ACTIVE = 11,
			GET_ACTIVE_SCENE = 12,
			MUTE_AUDIO_SOURCE = 13,
			UNMUTE_AUDIO_SOURCE = 14,
			HIDE_ITEM = 15,
			SHOW_ITEM = 16,
			SCENE_SWITCHED_SUBSCRIBE = 17,
			SCENE_ADDED_SUBSCRIBE = 18,
			SCENE_REMOVED_SUBSCRIBE = 19,
			SOURCE_ADDED_SUBSCRIBE = 20,
			SOURCE_REMOVED_SUBSCRIBE = 21,
			SOURCE_UPDATED_SUBSCRIBE = 22,
			ITEM_ADDED_SUBSCRIBED = 23,
			ITEM_REMOVED_SUBSCRIBE = 24,
			ITEM_UPDATED_SUBSCRIBE = 25,
			STREAMING_STATUS_CHANGED_SUBSCRIBE = 26,
			GET_ACTIVE_COLLECTION = 27,
			COLLECTION_ADDED_SUBSCRIBE = 28,
			COLLECTION_REMOVED_SUBSCRIBE = 29,
			COLLECTION_SWITCHED_SUBSCRIBE = 30,
			GET_RECORD_STREAM_STATE = 31,
			COLLECTION_UPDATED_SUBSCRIBE = 32,
			RECORDING_STATUS_CHANGED_SUBSCRIBE = 33,

			COUNT,
		};

	/*
	====================================================================================================
		Static Class Constants
	====================================================================================================
	*/
	private:

		static const byte EVENT_READ = 0x01;

		static const byte EVENT_WRITE = 0x02;

		static const byte EVENT_READ_WRITE = EVENT_READ | EVENT_WRITE;

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	public:

		static StreamdeckClient*
		createClient(qintptr socketDescriptor);

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::map<rpc_event, std::string> m_subscribedResources;

		byte m_authorizedEvents[1+(((int)rpc_event::COUNT-1)*2/8)];

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

		void
		close();

		void
		parse(
			const QJsonDocument& json_quest,
			Streamdeck::rpc_event& event,
			QString& service,
			QString& method,
			QVector<QVariant>& args
		);

		void
		send(const rpc_event event, const QJsonDocument& json_quest);

		bool
		sendSubscription(const rpc_event event, const std::string& resourceId);

		template<typename T>
		bool
		sendEvent(const rpc_event event, const T& data);

		bool
		sendEvent(const rpc_event event);

		bool
		sendRecordStreamState(
			const rpc_event event,
			const std::string& resourceId,
			const std::string& streaming,
			const std::string& recording
		);

		bool
		sendError(const rpc_event event, const std::string& resourceId, bool error);

		/*bool
		sendActiveCollectionMessage(
			const rpc_event event,
			const std::string& resourceId,
			const Collection* collection
		);

		bool
		sendCollectionSwitchMessage(const rpc_event event, const Collection* collection);

		bool
		sendCollectionsMessage(
			const rpc_event event,
			const std::string& resourceId,
			const Collections& collections
		);

		bool
		sendCollectionsSchema(
			const rpc_event event,
			const Collections& collections
		);

		bool
		sendScenesMessage(
			const rpc_event event,
			const std::string& resourceId,
			const Collection* collection,
			const Scenes& scenes
		);*/

		bool
		checkEventAuthorizations(const rpc_event event, byte flag);

		void
		setEventAuthorizations(const rpc_event event, byte flag);

		void
		lockEventAuthorizations(const rpc_event event);

		void
		unlockEventAuthorizations(const rpc_event event);

	private:

		void
		logEvent(const rpc_event event, const QJsonDocument& json_quest);

		QJsonObject
		buildJsonResponse(const rpc_event event, const QString& resourceId) const;

		QJsonObject
		buildJsonResult(const rpc_event event, const QString& resourceId) const;

		void
		addToJsonObject(QJsonObject& json_object, QString key, QJsonValue&& value) const;

		void
		addToJsonObject(QJsonValueRef&& json_object, QString key, QJsonValue&& value) const;

		void
		addToJsonArray(QJsonArray& json_array, QJsonValue&& value) const;

		void
		addToJsonArray(QJsonValueRef&& json_array, QJsonValue&& value) const;

	/*
	====================================================================================================
		Slots
	====================================================================================================
	*/
	public slots:

		void
		disconnected(int code);

		void
		read(QJsonDocument data);
	
	/*
	====================================================================================================
		Signals
	====================================================================================================
	*/
	signals:

		void
		close_client();

	signals:

		void
		clientDisconnected(Streamdeck*, int);

	signals:

		void
		received(
			Streamdeck* client,
			rpc_event event,
			QString service,
			QString method, 
			QVector<QVariant>,
			bool& error
		);

	signals:

		void
		write(QJsonDocument data);

};

/*
========================================================================================================
	Template Definitions
========================================================================================================
*/

#include "template/streamdeck/Streamdeck.tpp"