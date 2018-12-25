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

class CollectionsService : public ServiceImpl<CollectionsService> {

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	private:

		std::shared_ptr<Collection> m_collectionUpdated;

		uint16_t m_collectionToSwitch;

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
		onCollectionLoading(const obs::save::data& data);

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
		subscribeCollectionChange(const rpc::request& data);

		bool
		onFetchCollectionsSchema(const rpc::request& data);

		bool
		onGetCollections(const rpc::request& data);

		bool
		onGetActiveCollection(const rpc::request& data);

		bool
		onMakeCollectionActive(const rpc::request& data);

};