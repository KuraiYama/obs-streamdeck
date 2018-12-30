/*
 * Plugin Includes
 */
#include "include/services/RecordingService.hpp"
#include "include/common/Logger.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

RecordingService::RecordingService() :
	ServiceImpl("RecordingService", "StreamingService"),
	m_recordingOutput(nullptr) {
	this->setupEvent(obs::frontend::event::RECORDING_STARTING, &RecordingService::onRecordStarting);

	this->setupEvent(obs::frontend::event::RECORDING_STARTED, &RecordingService::onRecordStarted);

	this->setupEvent(obs::frontend::event::RECORDING_STOPPING, &RecordingService::onRecordStopping);

	this->setupEvent(obs::frontend::event::RECORDING_STOPPED, &RecordingService::onRecordStopped);

	this->setupEvent(
		rpc::event::RECORDING_STATUS_CHANGED_SUBSCRIBE,
		&RecordingService::subscribeRecordStatusChange
	);

	this->setupEvent(rpc::event::START_RECORDING, &RecordingService::startRecording);

	this->setupEvent(rpc::event::STOP_RECORDING, &RecordingService::stopRecording);

	this->setupEvent(obs::frontend::event::EXIT, &RecordingService::onExit);
}

RecordingService::~RecordingService() {
}

/*
========================================================================================================
	RPC Event Handling
========================================================================================================
*/

bool
RecordingService::subscribeRecordStatusChange(const rpc::request& data) {
	rpc::response<std::string> response = response_string(&data, "subscribeRecordStatusChange");
	if(data.event == rpc::event::RECORDING_STATUS_CHANGED_SUBSCRIBE) {
		response.event = rpc::event::RECORDING_STATUS_CHANGED_SUBSCRIBE;
		logInfo("Subscription to recording event required.");

		if(!checkResource(&data, QRegExp("(.+)"))) {
			// This streamdeck doesn't provide any resource to warn on stream state change
			logError("Streamdeck didn't provide resourceId to subscribe.");
			return false;
		}

		response.data = QString("%1.%2")
			.arg(data.serviceName.c_str())
			.arg(data.method.c_str())
			.toStdString();

		return streamdeckManager()->commit_to(response, &StreamdeckManager::setSubscription);
	}

	logError("subscribeRecordStatusChange not called by RECORDING_STATUS_CHANGED_SUBSCRIBED");
	return false;
}

bool
RecordingService::startRecording(const rpc::request& data) {
	rpc::response<rpc::response_error> response = response_error(&data, "startRecording");

	if(data.event == rpc::event::START_RECORDING) {
		response.event = rpc::event::START_RECORDING;
		logInfo("Streamdeck has required start recording...");
		if(checkResource(&data, QRegExp("startRecording"))) {
			if(obs_frontend_recording_active() == false) {
				obs_frontend_recording_start();
				// OBS triggers an event when record is started/stopped.
				// We use this event to detect if an error occured on starting/stopping record.
				// That's why we don't send any response to the streamdeck yet in that case.
				return true;
			}
		}
		logError("Error : RecordingService::startRecording not called by Streamdeck. "
			"Starting record aborted.");
	}
	else
		logError("startRecording not called by START_RECORDING");

	return streamdeckManager()->commit_to(response, &StreamdeckManager::setError);
}

bool
RecordingService::stopRecording(const rpc::request& data) {
	rpc::response<rpc::response_error> response = response_error(&data, "stopRecording");

	if(data.event == rpc::event::STOP_RECORDING) {
		response.event = rpc::event::STOP_RECORDING;
		logInfo("Streamdeck has required stop recording...");
		if(checkResource(&data, QRegExp("stopRecording"))) {
			if(obs_frontend_recording_active() == true) {
				obs_frontend_recording_stop();
				// OBS triggers an event when record is started/stopped.
				// We use this event to detect if an error occured on starting/stopping record.
				// That's why we don't send any response to the streamdeck yet in that case.
				return true;
			}
		}
		logError("Error : RecordingService::stopRecording not called by Streamdeck. "
			"Stopping record aborted.");
	}
	else
		logError("stopRecording not called by STOP_RECORDING");

	return streamdeckManager()->commit_to(response, &StreamdeckManager::setError);
}

/*
========================================================================================================
	OBS Singals Helpers
========================================================================================================
*/

bool
RecordingService::connectOutputHandler() {
	if(m_recordingOutput != nullptr)
		disconnectOutputHandler();
	m_recordingOutput = obs_frontend_get_recording_output();
	if(m_recordingOutput != nullptr) {
		signal_handler_t* signal_handler = obs_output_get_signal_handler(m_recordingOutput);
		if(signal_handler != nullptr) {
			signal_handler_connect(signal_handler, "starting",
				RecordingService::onRecordStarting, this);
			signal_handler_connect(signal_handler, "start",
				RecordingService::onRecordStarted, this);
			signal_handler_connect(signal_handler, "stopping",
				RecordingService::onRecordStopping, this);
			signal_handler_connect(signal_handler, "stop",
				RecordingService::onRecordStopped, this);
			obsManager()->registerOutput(m_recordingOutput);
			return true;
		}
	}
	return false;
}

void
RecordingService::disconnectOutputHandler() {
	if(m_recordingOutput != nullptr) {
		signal_handler_t* signal_handler = obs_output_get_signal_handler(m_recordingOutput);
		if(signal_handler != nullptr) {
			signal_handler_disconnect(signal_handler, "starting",
				RecordingService::onRecordStarting, this);
			signal_handler_disconnect(signal_handler, "start",
				RecordingService::onRecordStarted, this);
			signal_handler_disconnect(signal_handler, "stopping",
				RecordingService::onRecordStopping, this);
			signal_handler_disconnect(signal_handler, "stop",
				RecordingService::onRecordStopped, this);
		}
		obsManager()->unregisterOutput(m_recordingOutput);
		obs_output_release(m_recordingOutput);
		m_recordingOutput = nullptr;
	}
}

bool
RecordingService::checkOutput(calldata_t* data) const {
	obs_output_t* output = nullptr;
	calldata_get_ptr(data, "output", &output);
	if(output != m_recordingOutput) {
		logInfo("Error: RecordingOutput received by the callback is different from the "
			"registered one.");
		obs_frontend_recording_stop();
		return false;
	}

	return true;
}

/*
========================================================================================================
	OBS Event Handling
========================================================================================================
*/

bool
RecordingService::onExit() {
	disconnectOutputHandler();
	return true;
}


bool
RecordingService::onRecordStarting() {
	logInfo("OBS output is ready. OBS is launching record.");
	if(connectOutputHandler()) {
		return true;
	}

	logError("Error: Output is NULL.");
	obs_frontend_recording_stop();
	return false;
}

void
RecordingService::onRecordStarting(void* recordingService, calldata_t* data) {
	RecordingService* service = reinterpret_cast<RecordingService*>(recordingService);

	if(!service->checkOutput(data)) return;

	service->logInfo("OBS output is starting record.");
	rpc::response<std::string> response = service->response_string(nullptr, "onRecordingStarting");
	response.event = rpc::event::RECORDING_STATUS_CHANGED_SUBSCRIBE;
	response.data = "recording";
	service->streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);

	rpc::response<rpc::response_error> action = service->response_error(nullptr, "onRecordingStarting");
	action.event = rpc::event::START_RECORDING;
	action.data.error_flag = false;
	service->streamdeckManager()->commit_all(action, &StreamdeckManager::setError);
}

bool
RecordingService::onRecordStarted() {
	logInfo("OBS has started record.");
	return true;
}

void
RecordingService::onRecordStarted(void* recording_service, calldata_t* data) {
	RecordingService* service = reinterpret_cast<RecordingService*>(recording_service);

	if(!service->checkOutput(data)) return;

	service->logInfo("OBS output has started record");
	rpc::response<std::string> response = service->response_string(nullptr, "onRecordingStarted");
	response.event = rpc::event::RECORDING_STATUS_CHANGED_SUBSCRIBE;
	response.data = "starting";
	service->streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
RecordingService::onRecordStopping() {
	logInfo("OBS is stopping record.");
	return true;
}

void
RecordingService::onRecordStopping(void* recording_service, calldata_t* data) {
	RecordingService* service = reinterpret_cast<RecordingService*>(recording_service);

	if(!service->checkOutput(data)) return;

	service->logInfo("OBS output is stopping record");
	rpc::response<std::string> response = service->response_string(nullptr, "onRecordingStopping");
	response.event = rpc::event::RECORDING_STATUS_CHANGED_SUBSCRIBE;
	response.data = "stopping";
	service->streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
RecordingService::onRecordStopped() {
	logInfo("OBS has stopped record.");
	return true;
}

void
RecordingService::onRecordStopped(void* recording_service, calldata_t* data) {
	RecordingService* service = reinterpret_cast<RecordingService*>(recording_service);

	if(!service->checkOutput(data)) return;

	//service->disconnectOutputHandler();

	long long code = -1;
	calldata_get_int(data, "code", &code);

	service->logInfo("OBS output has stopped record.");
	rpc::response<std::string> response = service->response_string(nullptr, "onRecordingStopped");
	response.event = rpc::event::RECORDING_STATUS_CHANGED_SUBSCRIBE;
	response.data = "offline";
	service->streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);

	rpc::response<rpc::response_error> action = service->response_error(nullptr, "onRecordingStopped");
	if(code == 0) {
		action.event = rpc::event::STOP_RECORDING;
		action.data.error_flag = false;
	}
	else {
		action.event = rpc::event::START_RECORDING;
		action.data.error_flag = true;
	}
	service->streamdeckManager()->commit_all(action, &StreamdeckManager::setError);
}