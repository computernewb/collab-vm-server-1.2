//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#ifndef CORE_PLUGINMANAGER_H
#define CORE_PLUGINMANAGER_H

#include <filesystem>

namespace collab3::core {

	/**
	 * Manages core CollabVM Server plugins and user-installable plugins.
	 */
	struct PluginManager {
		bool Init();

		/**
		 * Safely unload all of the plugins.
		 */
		void UnloadPlugins();

		/**
		 * Load a plugin from the path.
		 * \param[in] path the path to the plugin.
		 */
		bool LoadPlugin(const std::filesystem::path& path);
	};

} // namespace collab3::core

#endif //CORE_PLUGINMANAGER_H
