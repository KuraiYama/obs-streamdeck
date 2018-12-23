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
		Types Definitions
	====================================================================================================
	*/
	private:

		class FileLoader {

			/*
			============================================================================================
				Instance Data Members
			============================================================================================
			*/
			private:

				std::fstream m_stream;

			/*
			============================================================================================
				Constructors / Destructor
			============================================================================================
			*/
			public:

				FileLoader(const char* filename, std::ios_base::openmode mode = std::ios::in);

				~FileLoader();

			/*
			============================================================================================
				Instance Methods
			============================================================================================
			*/
			public:

				bool
					open(const char* filename, std::ios_base::openmode mode);

				size_t
					read(char* buffer, size_t size);

				size_t
					write(char* buffer, size_t size);

		};

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

		std::string m_streamingState;

		std::string m_recordingState;

		obs_output_t* m_streamOutput;

		obs_output_t* m_recordOutput;

		InfoDialog* m_dialog;

		const char* m_database;
	
	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		ApplicationService(QMainWindow* parent, const char* database);

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
		checkOutput(calldata_t* data, obs_output_t* output) const;

		void
		addPluginWindows();

		uint16_t
		loadDatabase(OBSStorage<Collection>& collections);

		void
		saveDatabase();

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