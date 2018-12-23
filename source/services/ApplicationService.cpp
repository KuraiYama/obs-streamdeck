/*
 * Plugin Includes
 */
#include "include/services/ApplicationService.hpp"
#include "include/common/Logger.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

ApplicationService::FileLoader::FileLoader(const char* filename, std::ios_base::openmode mode) {
	try {
		if(open(filename, mode) == false) {
			m_stream.close();
			char message[1024];
			sprintf(message, "Error when loading file: %s", filename);
			throw std::exception(message);
		}
	}
	catch(std::exception& e) {
		throw std::exception(e.what());
	}
}

ApplicationService::FileLoader::~FileLoader() {
	if(m_stream.is_open())
		m_stream.close();
}

ApplicationService::ApplicationService(QMainWindow* parent, const char* database) :
	ServiceT("ApplicationService", ""),
	m_dialog(new InfoDialog(parent)),
	m_database(database),
	m_streamOutput(nullptr),
	m_recordOutput(nullptr) {
	m_streamingState = "offline";
	m_recordingState = "offline";

	/* OBS fully loaded event */
	this->setupEvent(OBS_FRONTEND_EVENT_FINISHED_LOADING, &ApplicationService::onApplicationLoaded);
	this->setupEvent(OBS_FRONTEND_EVENT_EXIT, &ApplicationService::onApplicationExit);
	this->setupEvent(OBS_FRONTEND_EVENT_STREAMING_LAUNCHING, &ApplicationService::onStreamLaunching);
	this->setupEvent(OBS_FRONTEND_EVENT_RECORDING_STARTING, &ApplicationService::onRecordStarting);

	this->setupEvent<const rpc_event_data&>(Streamdeck::rpc_event::GET_RECORD_STREAM_STATE,
		&ApplicationService::onGetRecordStreamState);
}

ApplicationService::~ApplicationService() {
	disconnectStreamOutputHandler();
	disconnectRecordOutputHandler();

	m_dialog->deleteLater();
}

/*
========================================================================================================
	Recording State Handler
========================================================================================================
*/

void
ApplicationService::onRecordStarting(void* streaming_service, calldata_t* data) {
	ApplicationService* service = reinterpret_cast<ApplicationService*>(streaming_service);
	if(!service->checkOutput(data, service->m_recordOutput)) return;
	service->m_recordingState = "recording";
}

void
ApplicationService::onRecordStarted(void* streaming_service, calldata_t* data) {
	ApplicationService* service = reinterpret_cast<ApplicationService*>(streaming_service);
	if(!service->checkOutput(data, service->m_recordOutput)) return;
	service->m_recordingState = "starting";
}

void
ApplicationService::onRecordStopping(void* streaming_service, calldata_t* data) {
	ApplicationService* service = reinterpret_cast<ApplicationService*>(streaming_service);
	if(!service->checkOutput(data, service->m_recordOutput)) return;
	service->m_recordingState = "stopping";
}

void
ApplicationService::onRecordStopped(void* streaming_service, calldata_t* data) {
	ApplicationService* service = reinterpret_cast<ApplicationService*>(streaming_service);
	if(!service->checkOutput(data, service->m_recordOutput)) return;
	service->m_recordingState = "offline";
	service->disconnectRecordOutputHandler();
}

/*
========================================================================================================
	Streaming State Handler
========================================================================================================
*/

void
ApplicationService::onStreamStarting(void* streaming_service, calldata_t* data) {
	ApplicationService* service = reinterpret_cast<ApplicationService*>(streaming_service);
	if(!service->checkOutput(data, service->m_streamOutput)) return;
	service->m_streamingState = "starting";
}

void
ApplicationService::onStreamStarted(void* streaming_service, calldata_t* data) {
	ApplicationService* service = reinterpret_cast<ApplicationService*>(streaming_service);
	if(!service->checkOutput(data, service->m_streamOutput)) return;
	service->m_streamingState = "live";
}

void
ApplicationService::onStreamStopping(void* streaming_service, calldata_t* data) {
	ApplicationService* service = reinterpret_cast<ApplicationService*>(streaming_service);
	if(!service->checkOutput(data, service->m_streamOutput)) return;
	service->m_streamingState = "ending";
}

void
ApplicationService::onStreamStopped(void* streaming_service, calldata_t* data) {
	ApplicationService* service = reinterpret_cast<ApplicationService*>(streaming_service);
	if(!service->checkOutput(data, service->m_streamOutput)) return;
	service->m_streamingState = "offline";
	service->disconnectStreamOutputHandler();
}

void
ApplicationService::onStreamReconnecting(void* streaming_service, calldata_t* data) {
	ApplicationService* service = reinterpret_cast<ApplicationService*>(streaming_service);
	if(!service->checkOutput(data, service->m_streamOutput)) return;
	service->m_streamingState = "reconnecting";
}

void
ApplicationService::onStreamReconnected(void* streaming_service, calldata_t* data) {
	ApplicationService* service = reinterpret_cast<ApplicationService*>(streaming_service);
	if(!service->checkOutput(data, service->m_streamOutput)) return;
	service->m_streamingState = "live";
}

/*
========================================================================================================
	OBS Signals Helpers
========================================================================================================
*/

bool
ApplicationService::connectStreamOutputHandler() {
	if(m_streamOutput != nullptr)
		disconnectStreamOutputHandler();
	m_streamOutput = obs_frontend_get_streaming_output();
	if(m_streamOutput != nullptr) {
		signal_handler_t* signal_handler = obs_output_get_signal_handler(m_streamOutput);
		if(signal_handler != nullptr) {
			signal_handler_connect(signal_handler, "starting",
				ApplicationService::onStreamStarting, this);
			signal_handler_connect(signal_handler, "start",
				ApplicationService::onStreamStarted, this);
			signal_handler_connect(signal_handler, "stopping",
				ApplicationService::onStreamStopping, this);
			signal_handler_connect(signal_handler, "stop",
				ApplicationService::onStreamStopped, this);
			signal_handler_connect(signal_handler, "reconnect",
				ApplicationService::onStreamReconnecting, this);
			signal_handler_connect(signal_handler, "reconnect_success",
				ApplicationService::onStreamReconnected, this);

			return true;
		}
	}
	return false;
}

bool
ApplicationService::connectRecordOutputHandler() {
	if(m_recordOutput != nullptr)
		disconnectRecordOutputHandler();
	m_recordOutput = obs_frontend_get_recording_output();
	if(m_recordOutput != nullptr) {
		signal_handler_t* signal_handler = obs_output_get_signal_handler(m_recordOutput);
		if(signal_handler != nullptr) {
			signal_handler_connect(signal_handler, "starting",
				ApplicationService::onRecordStarting, this);
			signal_handler_connect(signal_handler, "start",
				ApplicationService::onRecordStarted, this);
			signal_handler_connect(signal_handler, "stopping",
				ApplicationService::onRecordStopping, this);
			signal_handler_connect(signal_handler, "stop",
				ApplicationService::onRecordStopped, this);

			return true;
		}
	}
	return false;
}

void
ApplicationService::disconnectStreamOutputHandler() {
	if(m_streamOutput != nullptr) {
		signal_handler_t* signal_handler = obs_output_get_signal_handler(m_streamOutput);
		if(signal_handler != nullptr) {
			signal_handler_disconnect(signal_handler, "starting",
				ApplicationService::onStreamStarting, this);
			signal_handler_disconnect(signal_handler, "start",
				ApplicationService::onStreamStarted, this);
			signal_handler_disconnect(signal_handler, "stopping",
				ApplicationService::onStreamStopping, this);
			signal_handler_disconnect(signal_handler, "stop",
				ApplicationService::onStreamStopped, this);
			signal_handler_disconnect(signal_handler, "reconnect",
				ApplicationService::onStreamReconnecting, this);
			signal_handler_disconnect(signal_handler, "reconnect_success",
				ApplicationService::onStreamReconnected, this);
		}
		obs_output_release(m_streamOutput);
		m_streamOutput = nullptr;
	}
}

void
ApplicationService::disconnectRecordOutputHandler() {
	if(m_recordOutput != nullptr) {
		signal_handler_t* signal_handler = obs_output_get_signal_handler(m_recordOutput);
		if(signal_handler != nullptr) {
			signal_handler_disconnect(signal_handler, "starting",
				ApplicationService::onRecordStarting, this);
			signal_handler_disconnect(signal_handler, "start",
				ApplicationService::onRecordStarted, this);
			signal_handler_disconnect(signal_handler, "stopping",
				ApplicationService::onRecordStopping, this);
			signal_handler_disconnect(signal_handler, "stop",
				ApplicationService::onRecordStopped, this);
		}
		obs_output_release(m_recordOutput);
		m_recordOutput = nullptr;
	}
}

bool
ApplicationService::checkOutput(calldata_t* data, obs_output_t* output) const {
	obs_output_t* data_output = nullptr;
	calldata_get_ptr(data, "output", &data_output);
	return data_output == output;
}

/*
========================================================================================================
	OBS Event Helpers
========================================================================================================
*/

void
ApplicationService::addPluginWindows() {
	// setup tools menu action for show pluging info
	const char* label = obs_module_text("Elgato Remote Control for OBS Studio");
	QAction* action = (QAction*)obs_frontend_add_tools_menu_qaction(label);

	// Connect the new action to the pop dialog
	InfoDialog& dialog = *m_dialog;
	if(action != nullptr) {
		std::function<void(void)> f = [&dialog] {
			obs_frontend_push_ui_translation(obs_module_get_string);
			dialog.open();
			obs_frontend_pop_ui_translation();
		};
		action->connect(action, &QAction::triggered, f);
	}

	Logger::instance().output(dialog.logger());

	dialog.open();
}

bool
ApplicationService::onApplicationLoaded() {

	addPluginWindows();

	OBSStorage<Collection> collections;
	uint16_t last_collection_id = this->loadDatabase(collections);

	obsManager()->loadCollections(collections, last_collection_id);

	streamdeckManager()->listen();
	logInfo("Application Loaded.");
	return true;
}

bool
ApplicationService::onApplicationExit() {
	this->saveDatabase();
	disconnectStreamOutputHandler();
	disconnectRecordOutputHandler();
	return true;
}

bool
ApplicationService::onStreamLaunching() {
	connectStreamOutputHandler();
	return true;
}

bool
ApplicationService::onRecordStarting() {
	connectRecordOutputHandler();
	return true;
}

/*
========================================================================================================
	RPC Event Helpers
========================================================================================================
*/

bool
ApplicationService::onGetRecordStreamState(const rpc_event_data& data) {
	connectRecordOutputHandler();
	connectStreamOutputHandler();

	rpc_adv_response<std::pair<std::string,std::string>> response =
		response_string2(&data, "getRecordStreamState");

	if(data.event == Streamdeck::rpc_event::GET_RECORD_STREAM_STATE) {
		response.event = Streamdeck::rpc_event::GET_RECORD_STREAM_STATE;
		logInfo("Streamdeck has required record and stream state...");

		if(data.serviceName.compare("StreamingService") == 0 && data.method.compare("getModel") == 0) {
			response.data = std::make_pair(m_streamingState, m_recordingState);
			return streamdeckManager()->commit_to(response, &StreamdeckManager::setRecordStreamState);
		}

		logWarning("GetRecordStreamState - Unknown RPC Service");
		return true;
	}

	logError("GetRecordStreamState not called by GET_RCORD_STREAM_STATE.");
	return false;
}

/*
========================================================================================================
	Database Handling
========================================================================================================
*/

uint16_t
ApplicationService::loadDatabase(OBSStorage<Collection>& collections) {

	uint16_t collection_id = 0;

	try {
		FileLoader collections_file("collections.dat");

		// Read numbers of collections
		unsigned short collections_count = 0;
		collections_file.read((byte*)&collections_count, sizeof(unsigned short));

		while(collections_count > 0) {
			Collection* collection = nullptr;

			// Read block size
			size_t block_size = 0;
			collections_file.read((byte*)&block_size, sizeof(size_t));

			// Read block
			Memory block(block_size);
			collections_file.read(block, block_size);

			// Build collection (Threadable)
			collection = Collection::buildFromMemory(block);
			if(collection != nullptr) {
				collections.push(collection);
				collection_id = std::max<uint16_t>(collection_id, collection->id());
			}
			collections_count--;
		}
	}
	catch(std::exception& e) {
		log_error << QString("OBS Manager failed on loading file - %1").arg(e.what()).toStdString();
	}

	return collection_id;
}

void
ApplicationService::saveDatabase() {
	std::vector<Memory> collection_blocks;

	/*BLOCK
		nbCollections (short)
		foreach(collection)
			block_size (size_t)
			block_collection (block_size)
	*/
	Collections collections = obsManager()->collections();
	size_t size = sizeof(short);
	for(auto collection = collections.begin(); collection != collections.end(); collection++) {
		size += sizeof(size_t);
		collection_blocks.push_back((*collection)->toMemory(size));
	}

	short collections_count = static_cast<short>(collections.size());
	Memory block(size);

	// Write the number of collections
	block.write((byte*)&collections_count, sizeof(short));

	// For each collection
	for(auto iter = collection_blocks.begin(); iter < collection_blocks.end(); iter++) {
		size_t size_cl = iter->size();
		block.write((byte*)&size_cl, sizeof(size_t));
		block.write((*iter), iter->size());
	}

	try {
		FileLoader collections_file("collections.dat", std::ios::out);
		collections_file.write(block, block.size());
	}
	catch(std::exception& e) {
		log_error << QString("OBS Manager failed on writing file - %1").arg(e.what()).toStdString();
	}
}

/*
========================================================================================================
	File Handling
========================================================================================================
*/

bool
ApplicationService::FileLoader::open(const char* filename, std::ios_base::openmode mode) {
	if(m_stream.is_open())
		m_stream.close();
	m_stream.open(filename, mode | std::fstream::binary);
	if(m_stream.good()) {
		m_stream.exceptions(std::fstream::failbit | std::fstream::badbit | std::fstream::eofbit);
	}

	return m_stream.is_open();
}

size_t
ApplicationService::FileLoader::read(char* buffer, size_t size) {
	if(!m_stream.is_open() || !m_stream.good())
		return 0;

	try {
		m_stream.read(buffer, size);
	}
	catch(std::fstream::failure) {
		m_stream.close();
	}
	return m_stream.gcount();
}

size_t
ApplicationService::FileLoader::write(char* buffer, size_t size) {
	if(!m_stream.is_open() || !m_stream.good())
		return 0;
	try {
		m_stream.write(buffer, size);
	}
	catch(std::fstream::failure) {
		m_stream.close();
		return 0;
	}

	return size;
}