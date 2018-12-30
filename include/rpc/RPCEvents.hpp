#pragma once

/*
 * Std Includes
 */
#include <string>

/*
 * Qt Includes
 */
#include <QVariant>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class Streamdeck;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

namespace rpc {

	/*
	====================================================================================================
		RPC
	====================================================================================================
	*/

	enum class event {
		ERROR = -1,
		NO_EVENT = 0,
		START_STREAMING = 1,
		STOP_STREAMING = 2,
		START_RECORDING = 3,
		STOP_RECORDING = 4,
		GET_COLLECTIONS = 5,
		MAKE_COLLECTION_ACTIVE = 6,

		MISSING_NO = 7,

		FETCH_COLLECTIONS_SCHEMA = 8,
		GET_SCENES = 9,
		GET_SOURCES = 10,
		MAKE_SCENE_ACTIVE = 11,
		GET_ACTIVE_SCENE = 12,
		ENABLE_SOURCE = 13,
		DISABLE_SOURCE = 14,
		HIDE_ITEM = 15,
		SHOW_ITEM = 16,
		SCENE_SWITCHED_SUBSCRIBE = 17,
		SCENE_ADDED_SUBSCRIBE = 18,
		SCENE_REMOVED_SUBSCRIBE = 19,
		SOURCE_ADDED_SUBSCRIBE = 20,
		SOURCE_REMOVED_SUBSCRIBE = 21,
		SOURCE_UPDATED_SUBSCRIBE = 22,
		ITEM_ADDED_SUBSCRIBE = 23,
		ITEM_REMOVED_SUBSCRIBE = 24,
		ITEM_UPDATED_SUBSCRIBE = 25,
		STREAMING_STATUS_CHANGED_SUBSCRIBE = 26,
		GET_ACTIVE_COLLECTION = 27,
		COLLECTION_ADDED_SUBSCRIBE = 28,
		COLLECTION_REMOVED_SUBSCRIBE = 29,
		COLLECTION_SWITCHED_SUBSCRIBE = 30,
		GET_RECORD_STREAM_STATE = 31,
		COLLECTION_UPDATED_SUBSCRIBE = 32,
		RECORDING_STATUS_CHANGED_SUBSCRIBE = 33,

		COUNT,
	};

	struct request {
		event event;
		Streamdeck* client;
		const std::string serviceName;
		const std::string method;
		const QVector<QVariant> args;
	};

	struct response_base {
		const request* request;
		event event;
		const char* serviceName;
		const char* method;
	};

	template<typename T>
	struct response : response_base {
		T data;
	};

	template<>
	struct response<void> : response_base {
	};

	struct response_error {
		bool hasMessage;
		union {
			bool error_flag;
			const char* error_message;
		};
	};

}