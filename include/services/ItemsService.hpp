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

class ItemsService : public ServiceImpl<ItemsService> {

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		ItemsService();

		virtual ~ItemsService();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	private:

		bool
		subscribeItemChange(const rpc::request& data);

		bool
		onItemAdded(const obs::item::data& data);

		bool
		onItemRemoved(const obs::item::data& data);

		bool
		onItemUpdated(const obs::item::data& data);

		bool
		onItemChangeVisibility(const rpc::request& data);

};