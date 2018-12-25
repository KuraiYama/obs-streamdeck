#pragma once

/*
 * Qt Includes
 */
#include <QMap>

/*
 * OBS Includes
 */
#include <obs-frontend-api/obs-frontend-api.h>

/*
 * Plugin Includes
 */
#include "include/services/Service.hpp"
#include "include/events/EventObserver.hpp"
#include "include/streamdeck/StreamDeckManager.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class RecordingService : public ServiceImpl<RecordingService> {

	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	private:

		static void
		onRecordStarting(void* streaming_service, calldata_t* data);

		static void
		onRecordStarted(void* streaming_service, calldata_t* data);

		static void
		onRecordStopping(void* streaming_service, calldata_t* data);

		static void
		onRecordStopped(void* streaming_service, calldata_t* data);

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		obs_output_t* m_recordingOutput;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		RecordingService();

		virtual ~RecordingService();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	private:

		bool
		onExit();

		bool
		checkOutput(calldata_t* data) const;

		bool
		connectOutputHandler();

		void
		disconnectOutputHandler();

		bool
		onRecordStarting();

		bool
		onRecordStarted();

		bool
		onRecordStopping();

		bool
		onRecordStopped();

		bool
		startRecording(const rpc::request& data);

		bool
		stopRecording(const rpc::request& data);

		bool
		subscribeRecordStatusChange(const rpc::request& data);

};