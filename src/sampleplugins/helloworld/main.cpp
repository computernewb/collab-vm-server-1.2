//
// CollabVM Plugin Supplementary
//
// This file is licensed under the GNU Lesser General Public License Version 3
// for external usage in plugins. Text is provided in LICENSE-PLUGIN.
// If you cannot access it, https://www.gnu.org/licenses/lgpl-3.0.html
//

// A basic hello world plugin. Does not really implement anything useful.

#include <PluginAPI.h>

// symbol in AbiImplementation.cpp
extern collabvm::plugin::IPluginApi* g_PluginApi;

struct HelloWorld : public collabvm::plugin::IServerPlugin {

	void InitImpl() {
		g_PluginApi->WriteLogMessage(u8"Hello CollabVM World!");
	}

	HelloWorld() : IServerPlugin() {
		// assign vtfuncs here
		COLLABVM_PLUGINABI_ASSIGN_VTFUNC(Init, &HelloWorld::InitImpl);

		// Now it is safe for any caller to use those functions, even here!
	}

};

extern "C" {

COLLABVM_PLUGINABI_EXPORT collabvm::plugin::PluginMetadata* collabvm_plugin_get_metadata() {
	static collabvm::plugin::PluginMetadata data {
		.PluginName = u8"Hello World",
		.PluginDescription = u8"A programmer's delight!",
		.PluginAuthor = u8"CollabVM Core Team",
		.PluginLicense = u8"GNU LGPLv3",
		.PluginVersion = u8"N/A"
	};
	return &data;
}

COLLABVM_PLUGINABI_EXPORT collabvm::plugin::IServerPlugin* collabvm_plugin_make_serverplugin() {
	return new HelloWorld;
}

COLLABVM_PLUGINABI_EXPORT void collabvm_plugin_delete_serverplugin(collabvm::plugin::IServerPlugin* plugin) {
	if(!plugin)
		return;

	// This makes sure HelloWorld::~HelloWorld() is actually called,
	// if that's defined here (because we're not using classical C++ virtual functions.)
	return delete reinterpret_cast<HelloWorld*>(plugin);
}

}