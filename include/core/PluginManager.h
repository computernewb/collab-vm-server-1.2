#ifndef COLLAB_VM_SERVER_PLUGINMANAGER_H
#define COLLAB_VM_SERVER_PLUGINMANAGER_H

#include <filesystem>
#include <collabvm/plugin/PluginAPI.h>

namespace collabvm::core {

	/**
	 * Manages core CollabVM Server plugins and user-installable plugins.
	 */
	struct PluginManager {

		bool Init();

		/**
		 * Safely unload all of the plugins.
		 */
		void UnloadPlugins();

		plugin::IServerPlugin* GetServerPluginById(const plugin::utf8char* id);

		//plugin::ICorePlugin* GetCorePluginById(const plugin::utf8char* id);

		/**
		 * Get the currently configured DB Plugin instance.
		 */
		plugin::IDatabasePlugin* GetDbPlugin();


		/**
		 * Load a plugin from the path.
		 * \param[in] path the path to the plugin.
		 */
		bool LoadPlugin(const std::filesystem::path& path);


	};


}

#endif //COLLAB_VM_SERVER_PLUGINMANAGER_H
