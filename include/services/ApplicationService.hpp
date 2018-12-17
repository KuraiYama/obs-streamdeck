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
#include "include/ui/InfoDialog.h"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class ApplicationService : public ServiceT<ApplicationService> {

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	private:

		static void
		onRecordStarting(void* streamingService, calldata_t* data);

		static void
		onRecordStarted(void* streamingService, calldata_t* data);

		static void
		onRecordStopping(void* streamingService, calldata_t* data);

		static void
		onRecordStopped(void* streamingService, calldata_t* data);

		static void
		onStreamStarting(void* streamingService, calldata_t* data);

		static void
		onStreamStarted(void* streamingService, calldata_t* data);

		static void
		onStreamStopping(void* streamingService, calldata_t* data);

		static void
		onStreamStopped(void* streamingService, calldata_t* data);

		static void
		onStreamReconnecting(void* streamingService, calldata_t* data);

		static void
		onStreamReconnected(void* streamingService, calldata_t* data);

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		InfoDialog* m_dialog;

		std::string m_streamingState;

		std::string m_recordingState;

		obs_output_t* m_streamOutput;

		obs_output_t* m_recordOutput;
	
	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		ApplicationService(QMainWindow* parent);

		virtual ~ApplicationService();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	private:

		bool
		connectStreamOutputHandler();

		bool
		connectRecordOutputHandler();

		void
		disconnectStreamOutputHandler();

		void
		disconnectRecordOutputHandler();

		bool
		checkOutput(calldata_t* data, obs_output_t* output2) const;

		bool
		onApplicationLoaded();

		bool
		onApplicationExit();

		bool
		onStreamLaunching();
		
		bool
		onRecordStarting();

		bool
		onGetRecordStreamState(const rpc_event_data& data);

};