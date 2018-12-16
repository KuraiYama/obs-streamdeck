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

		obs_output_t* m_streamingOutput;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		StreamingService(StreamdeckManager* streamdeckManager);

		virtual ~StreamingService();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	private:

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

		bool
		checkOutput(calldata_t* data) const;

};