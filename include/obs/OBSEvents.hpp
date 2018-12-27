#pragma once

/*
========================================================================================================
	Types Predeclarations
========================================================================================================
*/

class Scene;
class Item;
class Source;

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

namespace obs {

	/*
	====================================================================================================
		Frontend
	====================================================================================================
	*/

	namespace frontend {

		enum class event {
			STREAMING_STARTING,
			STREAMING_LAUNCHING,
			STREAMING_STARTED,
			STREAMING_STOPPING,
			STREAMING_STOPPED,
			RECORDING_STARTING,
			RECORDING_STARTED,
			RECORDING_STOPPING,
			RECORDING_STOPPED,
			SCENE_CHANGED,
			SCENE_LIST_CHANGED,
			TRANSITION_CHANGED,
			TRANSITION_STOPPED,
			TRANSITION_LIST_CHANGED,
			SCENE_COLLECTION_CHANGED,
			SCENE_COLLECTION_LIST_CHANGED,
			PROFILE_CHANGED,
			PROFILE_LIST_CHANGED,
			EXIT,

			REPLAY_BUFFER_STARTING,
			REPLAY_BUFFER_STARTED,
			REPLAY_BUFFER_STOPPING,
			REPLAY_BUFFER_STOPPED,

			STUDIO_MODE_ENABLED,
			STUDIO_MODE_DISABLED,
			PREVIEW_SCENE_CHANGED,

			SCENE_COLLECTION_CLEANUP,
			FINISHED_LOADING,
			SCENE_COLLECTION_LOAD,
			SCENE_COLLECTION_CLEANED,
		};

	}

	/*
	====================================================================================================
		Save
	====================================================================================================
	*/

	namespace save {

		enum class event {
			SAVING,
			LOADING
		};

		typedef struct data {
			event event;
			obs_data_t* data;
		} data;

	}

	/*
	====================================================================================================
		Output
	====================================================================================================
	*/

	namespace output {

		enum class event {
			STARTING,
			STARTED,
			STOPPING,
			STOPPED,
			RECONNECTING,
			RECONNECTED
		};

		typedef struct data {
			event event;
			obs_output_t* output;
			const char* state;
		} data;

	}

	/*
	====================================================================================================
		Collection
	====================================================================================================
	*/

	namespace collection {

		enum class event {
			ADDED = 0,
			REMOVED,
			RENAMED,
			SWITCHED,

			LIST_BUILD,
		};

	}

	/*
	====================================================================================================
		Scene
	====================================================================================================
	*/

	namespace scene {

		enum class event {
			ADDED = 0,
			REMOVED,
			RENAMED,
			SWITCHED,

			LIST_BUILD,
		};

	}

	/*
	====================================================================================================
		Item
	====================================================================================================
	*/

	namespace item {

		enum class event {
			ADDED = 0,
			REMOVED,
			SHOWN,
			HIDDEN,

			LIST_BUILD,
		};

		typedef struct data {
			event event;
			Scene* scene;
			union {
				Item* item;
				obs_sceneitem_t* sceneitem;
			};
		} data;

	}

	/*
	====================================================================================================
		Source
	====================================================================================================
	*/

	namespace source {

		enum class event {
			ADDED = 0,
			REMOVED,
			RENAMED,
			MUTE,
			FLAGS,
		};

		typedef struct data {
			event event;
			Source* source;
			union {
				obs_source_t* obs_source;
				bool boolean_value;
				const char* string_value;
				int64_t uint_value;
			} data;
		} data;

	}

}