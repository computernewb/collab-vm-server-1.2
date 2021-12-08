#ifndef COLLAB_VM_SERVER_PLUGINMANAGER_H
#define COLLAB_VM_SERVER_PLUGINMANAGER_H

#include <plugin/PluginAPI.h>

namespace collabvm::core {

	/**
	 * Manages core plugins and user-defined plugins.
	 */
	struct PluginManager {

		bool Init();

		plugin::IServerPlugin* GetPluginById(const plugin::utf8char* id);


		void LoadPlugin();


	   private:

	};


}

#endif //COLLAB_VM_SERVER_PLUGINMANAGER_H
