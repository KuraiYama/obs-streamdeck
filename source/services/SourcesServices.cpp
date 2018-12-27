/*
 * Plugin Includes
 */
#include "include/services/SourcesService.hpp"
#include "include/common/Logger.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

SourcesService::SourcesService() :
	ServiceImpl("SourcesService", "SourcesService") {

	this->setupEvent(rpc::event::SOURCE_ADDED_SUBSCRIBE, &SourcesService::subscribeSourceChange);

	this->setupEvent(rpc::event::SOURCE_REMOVED_SUBSCRIBE, &SourcesService::subscribeSourceChange);

	this->setupEvent(rpc::event::SOURCE_UPDATED_SUBSCRIBE, &SourcesService::subscribeSourceChange);

	this->setupEvent(rpc::event::GET_SOURCES, &SourcesService::onGetSources);

	this->setupEvent(obs::source::event::ADDED, &SourcesService::onSourceAdded);

	this->setupEvent(obs::source::event::REMOVED, &SourcesService::onSourceRemoved);

	this->setupEvent(obs::source::event::MUTE, &SourcesService::onSourceMuted);

	this->setupEvent(obs::source::event::RENAMED, &SourcesService::onSourceRenamed);

	this->setupEvent(obs::source::event::FLAGS, &SourcesService::onSourceFlagsChanged);

}

SourcesService::~SourcesService() {
}

/*
========================================================================================================
	OBS Event Handling
========================================================================================================
*/

bool
SourcesService::onSourceAdded(const obs::source::data& data) {

	// Loading collection - nothing to do
	if(obsManager()->isLoadingCollection() || obsManager()->activeCollection() == nullptr)
		return true;

	// Scene are handled by ScenesService
	if(strcmp(obs_source_get_id(data.data.obs_source), "scene") == 0) {
		return true;
	}

	if(data.data.obs_source == nullptr) {
		logError("Something went wrong on source creation.");
		return false;
	}

	Source* source = obsManager()->activeCollection()->addSource(data.data.obs_source);
	obsManager()->registerSource(source);

	logInfo(QString("Source %1 created.")
		.arg(source->name().c_str())
		.toStdString()
	);

	rpc::response<void> response = response_void(nullptr, "onSourceAdded");
	response.event = rpc::event::SOURCE_ADDED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
SourcesService::onSourceRemoved(const obs::source::data& data) {

	// Loading collection - nothing to do
	if(obsManager()->isLoadingCollection() || obsManager()->activeCollection() == nullptr)
		return true;

	if(data.source->collection() != obsManager()->activeCollection()) {
		logError("The source is not owned by the current collection. Cannot be destroyed.");
		return false;
	}

	obsManager()->unregisterSource(data.source);
	std::shared_ptr<Source> source_ref = obsManager()->activeCollection()->removeSource(*data.source);

	logInfo(QString("Source %1 removed from current collection.")
		.arg(source_ref->name().c_str())
		.toStdString()
	);

	rpc::response<void> response = response_void(nullptr, "onSourceRemoved");
	response.event = rpc::event::SOURCE_REMOVED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
SourcesService::onSourceRenamed(const obs::source::data& data) {

	// Loading collection - nothing to do
	if(obsManager()->isLoadingCollection() || obsManager()->activeCollection() == nullptr)
		return true;

	if(data.source->collection() != obsManager()->activeCollection()) {
		logError("The source is not owned by the current collection. Cannot be renamed.");
		return false;
	}

	logInfo(QString("Source %1 renamed to %2.")
		.arg(data.source->name().c_str())
		.arg(data.data.string_value)
		.toStdString()
	);

	obsManager()->activeCollection()->renameSource(*data.source, data.data.string_value);

	rpc::response<void> response = response_void(nullptr, "onSourceRenamed");
	response.event = rpc::event::SOURCE_UPDATED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
SourcesService::onSourceMuted(const obs::source::data& data) {

	// Loading collection - nothing to do
	if(obsManager()->isLoadingCollection() || obsManager()->activeCollection() == nullptr)
		return true;

	if(data.source->collection() != obsManager()->activeCollection()) {
		logError("The source is not owned by the current collection. Cannot be renamed.");
		return false;
	}

	data.source->muted(data.data.boolean_value);

	if(data.source->muted()) {
		logInfo(QString("Source %1 muted.")
			.arg(data.source->name().c_str())
			.toStdString()
		);
	}
	else {
		logInfo(QString("Source %1 unmuted.")
			.arg(data.source->name().c_str())
			.toStdString()
		);
	}

	rpc::response<void> response = response_void(nullptr, "onSourceMuted");
	response.event = rpc::event::SOURCE_UPDATED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

bool
SourcesService::onSourceFlagsChanged(const obs::source::data& data) {

	// Loading collection - nothing to do
	if(obsManager()->isLoadingCollection() || obsManager()->activeCollection() == nullptr)
		return true;

	if(data.source->collection() != obsManager()->activeCollection()) {
		logError("The source is not owned by the current collection. Cannot be renamed.");
		return false;
	}

	data.source->audio(data.data.uint_value);

	if(data.source->audio()) {
		logInfo(QString("Source %1 : Audio enabled.")
			.arg(data.source->name().c_str())
			.toStdString()
		);
	}
	else {
		logInfo(QString("Source %1 : Audio disabled.")
			.arg(data.source->name().c_str())
			.toStdString()
		);
	}

	rpc::response<void> response = response_void(nullptr, "onSourceFlags");
	response.event = rpc::event::SOURCE_UPDATED_SUBSCRIBE;

	return streamdeckManager()->commit_all(response, &StreamdeckManager::setEvent);
}

/*
========================================================================================================
	RPC Event Handling
========================================================================================================
*/

bool
SourcesService::subscribeSourceChange(const rpc::request& data) {
	rpc::response<std::string> response = response_string(&data, "subscribeSourceChange");
	if(data.event == rpc::event::SOURCE_ADDED_SUBSCRIBE ||
		data.event == rpc::event::SOURCE_REMOVED_SUBSCRIBE ||
		data.event == rpc::event::SOURCE_UPDATED_SUBSCRIBE
		) {
		response.event = data.event;
		logInfo("Subscription to source event required");

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

	logError("subscribeSourceChange not called by SOURCE_SUBSCRIBE");
	return false;
}

bool
SourcesService::onGetSources(const rpc::request& data) {

	rpc::response<Sources> response = response_sources(&data, "onGetSources");

	if(data.event == rpc::event::GET_SOURCES) {
		response.event = rpc::event::GET_SOURCES;
		logInfo("Sources list required.");

		if(!checkResource(&data, QRegExp("getSources"))) {
			logWarning("Unknown resource for getSources.");
		}

		if(data.args.size() <= 0) {
			logError("No argument provided by get_sources.");
			return false;
		}

		Collection* collection = nullptr;
		if(data.args[0].compare("") == 0) {
#if defined(COMPLETE_MODE)
			Collections collections = obsManager()->collections();
			bool result = true;
			for(auto iter = collections.begin(); iter < collections.end() && result; iter++) {
				response.data = (*iter)->sources();
				result &= streamdeckManager()->commit_to(response, &StreamdeckManager::setSources);
			}
			return result;
#else
			collection = obsManager()->activeCollection();
#endif
		}
		else {
			uint16_t id = QString(data.args[0].toString()).toShort();
			collection = obsManager()->collection(id);
		}

		if(collection != nullptr)
			response.data = collection->sources();

		return streamdeckManager()->commit_to(response, &StreamdeckManager::setSources);
	}

	logError("getScenes not called by GET_SOURCES");
	return false;
}
