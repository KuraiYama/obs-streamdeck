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
#include "include/rpc/RPCEvents.hpp"
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

		static bool _is_verbose;

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

		StreamdeckClient(qintptr socket_descriptor);

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
		createClient(qintptr socket_descriptor);

		static QJsonObject
		buildJsonResponse(const rpc::event event, const QString& resource, bool event_mode = false);

		static QJsonObject
		buildJsonResult(const rpc::event event, const QString& resource, bool event_mode = false);

		static void
		addToJsonObject(QJsonObject& json_object, QString key, QJsonValue&& value);

		static void
		addToJsonObject(QJsonValueRef&& json_object, QString key, QJsonValue&& value);

		static void
		addToJsonArray(QJsonArray& json_array, QJsonValue&& value);

		static void
		addToJsonArray(QJsonValueRef&& json_array, QJsonValue&& value);

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::map<rpc::event, std::string> m_subscribedResources;

		byte m_authorizedEvents[1+(((int)rpc::event::COUNT-1)*2/8)];

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
			rpc::event& event,
			QString& service,
			QString& method,
			QVector<QVariant>& args
		);

		void
		send(
			const rpc::event event,
			const QJsonDocument& json_quest
		);

		bool
		sendAcknowledge(
			const rpc::event event,
			const std::string& resource,
			bool event_mode = false
		);

		template<typename T>
		bool
		sendResult(
			const rpc::event event,
			const std::string& resource,
			const T& data,
			bool event_mode = false
		);

		bool
		sendSubscription(
			const rpc::event event,
			const std::string& resource,
			bool event_mode = false
		);

		template<typename T>
		bool
		sendEvent(
			const rpc::event event,
			const T& data,
			bool event_mode = false
		);

		bool
		sendEvent(
			const rpc::event event,
			bool event_mode = false
		);

		bool
		sendRecordStreamState(
			const rpc::event event,
			const std::string& resource,
			const std::string& streaming,
			const std::string& recording,
			bool event_mode = false
		);

		bool
		sendError(
			const rpc::event event,
			const std::string& resource,
			bool error,
			bool event_mode = false
		);

		bool sendSchema(
			const rpc::event event,
			const std::string& resource,
			const Collections& collections,
			bool event_mode = false
		);

		bool
		sendCollections(
			const rpc::event event,
			const std::string& resource,
			const Collections& collections,
			bool event_mode = false
		);

		bool
		sendCollection(
			const rpc::event event,
			const std::string& resource,
			const CollectionPtr& collection,
			bool event_mode = false
		);

		bool
		sendScenes(
			const rpc::event event,
			const std::string& resource,
			const Scenes& scenes,
			bool event_mode = false
		);

		bool
		sendScene(
			const rpc::event event,
			const std::string& resource,
			const ScenePtr& scene,
			bool event_mode = false
		);

		bool
		checkEventAuthorizations(
			const rpc::event event,
			byte flag
		);

		void
		setEventAuthorizations(
			const rpc::event event,
			byte flag
		);

		void
		lockEventAuthorizations(const rpc::event event);

		void
		unlockEventAuthorizations(const rpc::event event);

	private:

		void
		logEvent(const rpc::event event, const QJsonDocument& json_quest);

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
			rpc::event event,
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