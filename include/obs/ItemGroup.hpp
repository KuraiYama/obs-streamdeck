#pragma once

/*
 * Plugin Includes
 */
#include "include/obs/Item.hpp"

/*
========================================================================================================
	Types Definitions
========================================================================================================
*/

class ItemGroup :
	public Item {

	/*
	====================================================================================================
		Static Class Attributes
	====================================================================================================
	*/
	public:

		static bool _toggle_subitems;

	/*
	====================================================================================================
		Instance Data Members
	====================================================================================================
	*/
	private:

		std::set<Item*> m_items;

		bool m_toggling;

	/*
	====================================================================================================
		Constructors / Destructor
	====================================================================================================
	*/
	public:

		ItemGroup(Scene* scene, uint16_t id, obs_sceneitem_t* item);

		ItemGroup(Scene* scene, uint16_t id, const std::string& name);

		virtual ~ItemGroup();

	/*
	====================================================================================================
		Instance Methods
	====================================================================================================
	*/
	public:

		void
		item(obs_sceneitem_t* item) override;

		const char*
		type() const override;

		void
		remove(Item* item);

		void
		add(Item* item);

		bool
		visible() const override;

		bool
		visible(bool toggle, bool rpc_action = false) override;

};