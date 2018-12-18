#pragma once

/*
 * Qt Includes
 */
#include <QMap>
#include <QMainWindow>
#include <QAction>

/*
 * OBS Includes
 */
#include <obs.h>
#include <obs-frontend-api/obs-frontend-api.h>
#include <obs-module.h>

/*
 * Plugin Includes
 */
#include "include/services/Service.hpp"
#include "include/streamdeck/StreamdeckManager.hpp"
#include "include/obs/Collection.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class CollectionsService : public ServiceT<CollectionsService> {

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		CollectionsService();

		virtual ~CollectionsService();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	private:

		bool
		onCollectionsListChanged();

		bool
		onCollectionAdded(const Collection& collection);

		bool
		onCollectionRemoved(const Collection& collection);

		bool
		onCollectionUpdated(const Collection& collection);

		bool
		onCollectionSwitched();

		bool
		subscribeCollectionChange(const rpc_event_data& data);

		bool
		onFetchCollectionsSchema(const rpc_event_data& data);

		bool
		onGetCollections(const rpc_event_data& data);

		bool
		onGetActiveCollection(const rpc_event_data& data);

		/*bool
		onMakeCollectionActive(const rpc_event_data& data);*/

};