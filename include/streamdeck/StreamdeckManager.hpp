#pragma once

/*
 * Qt Includes
 */
#include <QMainWindow>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QtNetwork>
#include <QLocalSocket>
#include <QSet>

/*
 * Plugin Includes
 */
#include "include/common/Global.h"
#include "include/events/EventObservable.hpp"
#include "include/streamdeck/Streamdeck.hpp"
#include "include/obs/Collection.hpp"

/*
========================================================================================================
	Defines
========================================================================================================
*/

#define OBS_PORT 28195

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class StreamdeckManager;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

struct rpc_event_data {
	Streamdeck::rpc_event event;
	Streamdeck* client;
	const std::string serviceName;
	const std::string method;
	const QVector<QVariant> args;
};

struct rpc_response {
	const rpc_event_data* request;
	Streamdeck::rpc_event event;
	const char* serviceName;
	const char* method;
};

template<typename T>
struct rpc_adv_response : rpc_response {
	T data;
};

template<>
struct rpc_adv_response<void> : rpc_response {
};


class StreamdeckServer : private QTcpServer {
	
	friend class StreamdeckManager;

	Q_OBJECT

	/*
	============================================================================================
		Instance Data Members
	============================================================================================
	*/
	private:

		QMap<QTcpSocket*, StreamdeckClient*> m_pendingClients;

	/*
	============================================================================================
		Constructors / Destructor
	============================================================================================
	*/
	private:

		StreamdeckServer(QObject* parent);

		virtual ~StreamdeckServer();

	/*
	============================================================================================
		Instance Methods
	============================================================================================
	*/
	private:

		void
		incomingConnection(qintptr socketDescriptor) override final;

	public:

		StreamdeckClient*
		nextPendingClient();

};

class StreamdeckManager : public QObject, public SafeEventObservable<Streamdeck::rpc_event> {
	
	Q_OBJECT

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:
		
		StreamdeckServer m_internalServer;

		QSet<Streamdeck*> m_streamdecks;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		StreamdeckManager(short listenPort = OBS_PORT);
		
		~StreamdeckManager();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		template<typename T>
		bool
		commit_to(
			rpc_adv_response<T>& response,
			bool(StreamdeckManager::*functor)(Streamdeck*, const rpc_adv_response<T>&)
		);

		template<typename T>
		bool
		commit_all(
			rpc_adv_response<T>& response,
			bool(StreamdeckManager::*functor)(Streamdeck*, const rpc_adv_response<T>&)
		);

		template<typename T>
		bool
		commit_any(
			rpc_adv_response<T>& response,
			bool(StreamdeckManager::*functor)(Streamdeck*, const rpc_adv_response<T>&)
		);

		bool
		setSubscription(Streamdeck* client, const rpc_adv_response<std::string>& reponse);

		template<typename T>
		bool
		setEvent(Streamdeck* client, const rpc_adv_response<T>& status);

		bool
		setRecordStreamState(
			Streamdeck* client, 
			const rpc_adv_response<std::pair<std::string, std::string>>& response
		);

		bool
		setError(Streamdeck* client, const rpc_adv_response<bool>& response);

		bool
		setSchema(Streamdeck* client, const rpc_adv_response<Collections>& response);

		bool
		setCollections(Streamdeck* client, const rpc_adv_response<Collections>& response);

		bool
		setCollection(Streamdeck* client, const rpc_adv_response<CollectionPtr>& response);

		/*bool
		setCollectionSwitched(
			Streamdeck* client,
			const rpc_adv_response<Collection*>& response
		);

		bool
		setScenes(
			Streamdeck* client,
			const rpc_adv_response<std::tuple<Collection*,Scenes>>& response
		);
		*/
		
	private:

		void
		close(Streamdeck* streamdeck);

		bool
		validate(rpc_response& response);

	/*
	====================================================================================================
		Slots
	====================================================================================================
	*/
	private slots:

		void
		onClientConnected();

		void
		onClientDisconnected(Streamdeck* streamdeck, int code);

		void
		receiveMessage(
			Streamdeck* streamdeck,
			Streamdeck::rpc_event event,
			QString service, 
			QString method,
			QVector<QVariant> args,
			bool& error
		);

};

/*
========================================================================================================
	Template Definitions
========================================================================================================
*/

#include "template/streamdeck/StreamdeckManager.tpp"