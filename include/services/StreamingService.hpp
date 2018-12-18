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

class StreamingService : public ServiceT<StreamingService> {
	
	/*
	====================================================================================================
		Static Class Functions
	====================================================================================================
	*/
	private:

		static void
		onStreamStarting(void* streaming_service, calldata_t* data);

		static void
		onStreamStarted(void* streaming_service, calldata_t* data);

		static void
		onStreamStopping(void* streaming_service, calldata_t* data);

		static void
		onStreamStopped(void* streaming_service, calldata_t* data);

		static void
		onStreamReconnecting(void* streaming_service, calldata_t* data);

		static void
		onStreamReconnected(void* streaming_service, calldata_t* data);

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		obs_output_t* m_streamingOutput;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		StreamingService();

		virtual ~StreamingService();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	private:

		bool
		checkOutput(calldata_t* data) const;

		bool
		connectOutputHandler();

		void
		disconnectOutputHandler();

		bool
		onStreamStarting();

		bool
		onStreamLaunching();

		bool
		onStreamStarted();

		bool
		onStreamStopping();

		bool
		onStreamStopped();

		bool
		startStreaming(const rpc_event_data& data);

		bool
		stopStreaming(const rpc_event_data& data);

		bool
		subscribeStreamStatusChange(const rpc_event_data& data);

};