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

class SourcesService : public ServiceImpl<SourcesService> {

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		SourcesService();

		virtual ~SourcesService();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	private:

		bool
		subscribeSourceChange(const rpc::request& data);

		bool
		onSourceAdded(const obs::source::data& data);

		bool
		onSourceRemoved(const obs::source::data& data);

		bool
		onSourceRenamed(const obs::source::data& data);

		bool
		onSourceMuted(const obs::source::data& data);

		bool
		onSourceFlagsChanged(const obs::source::data& data);

		bool
		onGetSources(const rpc::request& data);

};