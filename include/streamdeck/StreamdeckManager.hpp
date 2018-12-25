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
#include "include/Global.h"
#include "include/events/EventObservable.hpp"
#include "include/streamdeck/Streamdeck.hpp"
#include "include/obs/Collection.hpp"
#include "include/obs/Scene.hpp"

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
		incomingConnection(qintptr socket_descriptor) override final;

	public:

		StreamdeckClient*
		nextPendingClient();

};

class StreamdeckManager : public QObject, public SafeEventObservable<rpc::event> {
	
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

		StreamdeckManager();
		
		~StreamdeckManager();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void
		listen(short listen_port = OBS_PORT);

		template<typename T>
		bool
		commit_to(
			rpc::response<T>& response,
			bool(StreamdeckManager::*functor)(Streamdeck*, const rpc::response<T>&)
		);

		template<typename T>
		bool
		commit_all(
			rpc::response<T>& response,
			bool(StreamdeckManager::*functor)(Streamdeck*, const rpc::response<T>&)
		);

		template<typename T>
		bool
		commit_any(
			rpc::response<T>& response,
			bool(StreamdeckManager::*functor)(Streamdeck*, const rpc::response<T>&)
		);

		bool
		setAcknowledge(Streamdeck* client, const rpc::response<void>& response);

		bool
		setSubscription(Streamdeck* client, const rpc::response<std::string>& response);

		template<typename T>
		bool
		setResult(Streamdeck* client, const rpc::response<T>& response);

		template<typename T>
		bool
		setEvent(Streamdeck* client, const rpc::response<T>& response);

		bool
		setRecordStreamState(
			Streamdeck* client, 
			const rpc::response<std::pair<std::string, std::string>>& response
		);

		bool
		setError(Streamdeck* client, const rpc::response<bool>& response);

		bool
		setSchema(Streamdeck* client, const rpc::response<Collections>& response);

		bool
		setCollections(Streamdeck* client, const rpc::response<Collections>& response);

		bool
		setCollection(Streamdeck* client, const rpc::response<CollectionPtr>& response);

		bool
		setScenes(Streamdeck* client, const rpc::response<Scenes>& response);

		bool
		setScene(Streamdeck* client, const rpc::response<ScenePtr>& response);
		
	private:

		void
		close(Streamdeck* streamdeck);

		bool
		validate(rpc::response_base& response);

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
			rpc::event event,
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