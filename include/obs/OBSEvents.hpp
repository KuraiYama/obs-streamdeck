#pragma once

namespace obs {

	/*
	====================================================================================================
		Types Definitions
	====================================================================================================
	*/

	enum class collection_event {
		OBS_COLLECTION_ADDED = 0,
		OBS_COLLECTION_REMOVED,
		OBS_COLLECTION_RENAMED,
		OBS_COLLECTION_SWITCHED,

		OBS_COLLECTIONS_LIST_BUILD,
	};

	enum class scene_event {
		OBS_SCENE_ADDED = 0,
		OBS_SCENE_REMOVED,
		OBS_SCENE_RENAMED,
		OBS_SCENE_SWITCHED,

		OBS_SCENES_LIST_BUILD,
	};

}