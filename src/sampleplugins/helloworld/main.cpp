//
// CollabVM Plugin Supplementary
//
// This file is licensed under the GNU Lesser General Public License Version 3
// for external usage in plugins. Text is provided in LICENSE-PLUGIN.
// If you cannot access it, https://www.gnu.org/licenses/lgpl-3.0.html
//

// A basic hello world plugin. Does not really implement anything useful.

#include <PluginAPI.h>

// Symbols imported or are expected to exist for the
// common plugin code. Only touch these if you're writing
// a core plugin (e.g: Core VM or Display plugin.)
extern collabvm::plugin::IPluginApi* g_PluginApi;
bool g_IsCorePlugin = false;

// The plugin implementation class.
struct HelloWorld : public collabvm::plugin::IServerPlugin {

	void InitImpl() {
		g_PluginApi->WriteLogMessage(u8"Hello, CollabVM World!");
	}

	HelloWorld() : IServerPlugin() {
		// Assign implementation functions used
		COLLABVM_PLUGINABI_ASSIGN_VTFUNC(Init, &HelloWorld::InitImpl);

		// Any pre-initialization your plugin
	}

	~HelloWorld() {
		g_PluginApi->WriteLogMessage(u8"Goodbye, CollabVM World!");
		// Your plugin can close any resources it has opened here as well.
	}

};

extern "C" {

COLLABVM_PLUGINABI_EXPORT collabvm::plugin::PluginMetadata* collabvm_plugin_get_metadata() {
	static collabvm::plugin::PluginMetadata data {
		.PluginId = u8"helloworld",
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
	// because we're not using classical C++ virtual functions.
	return delete reinterpret_cast<HelloWorld*>(plugin);
}

}