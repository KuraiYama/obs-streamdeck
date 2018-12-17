#pragma once

namespace obs {

	/*
	====================================================================================================
		Types Definitions
	====================================================================================================
	*/

	enum class collection_event {
		COLLECTION_ADDED = 0,
		COLLECTION_REMOVED,
		COLLECTION_RENAMED,
		COLLECTION_SWITCHED,

		COLLECTIONS_LIST_BUILD,
	};

	enum class scene_event {
		SCENE_ADDED = 0,
		SCENE_REMOVED,
		SCENE_RENAMED,
		SCENE_SWITCHED,

		SCENES_LIST_BUILD,
	};

}