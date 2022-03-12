//
// CollabVM Plugin Supplementary
//
// This file is licensed under the GNU Lesser General Public License Version 3
// for external usage in plugins. Text is provided in LICENSE-PLUGIN.
// If you cannot access it, please see https://www.gnu.org/licenses/lgpl-3.0.html.
//

// A basic hello world plugin. Does not really implement anything useful.

#include <PluginAPI.h>

// Symbols imported or are expected to exist for the
// common plugin code.
extern collabvm::plugin::IPluginApi* g_PluginApi;

// The plugin implementation class.
struct HelloWorld : public collabvm::plugin::IServerPlugin {

	void InitImpl() {
		g_PluginApi->WriteLogMessage(collabvm::plugin::IPluginApi::LogLevel::Info, u8"Hello, CollabVM World!");
	}

	HelloWorld() : IServerPlugin() {
		// Assign implementation functions for this plugin here:
		COLLABVM_PLUGINABI_ASSIGN_VTFUNC(Init, &HelloWorld::InitImpl);

		// Any pre-initialization your plugin needs to do can be implemented here.
	}

	~HelloWorld() {
		g_PluginApi->WriteLogMessage(collabvm::plugin::IPluginApi::LogLevel::Info, u8"Goodbye, CollabVM World!");
		// Your plugin can close any resources it has opened here as well.
	}

};

extern "C" {

// NOTE: couldn't this be implemented in the ABI impl?
// just ask for a global metadata symbol there

COLLABVM_PLUGINABI_EXPORT collabvm::plugin::PluginMetadata* collabvm_plugin_get_metadata() {
	// Fill out metadata for your plugin here.
	// Make sure the ID is unique!
	static collabvm::plugin::PluginMetadata metadata {
		.PluginId = u8"helloworld",
		.PluginName = u8"Hello World",
		.PluginDescription = u8"A programmer's delight!",
		.PluginAuthor = u8"CollabVM Core Team",
		.PluginLicense = u8"GNU LGPLv3",
		.PluginVersion = u8"N/A"
	};
	return &metadata;
}

COLLABVM_PLUGINABI_EXPORT collabvm::plugin::IServerPlugin* collabvm_plugin_make_serverplugin() {
	return g_PluginApi->New<HelloWorld>();
}

COLLABVM_PLUGINABI_EXPORT void collabvm_plugin_delete_serverplugin(collabvm::plugin::IServerPlugin* plugin) {
	g_PluginApi->Delete(reinterpret_cast<HelloWorld*>(plugin));
}

}