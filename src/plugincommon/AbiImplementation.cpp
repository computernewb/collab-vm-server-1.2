//
// CollabVM Plugin Supplementary
//
// This file is licensed under the GNU Lesser General Public License Version 3
// for external usage in plugins. Text is provided in LICENSE-PLUGIN.
// If you cannot access it, please see https://www.gnu.org/licenses/lgpl-3.0.html.
//

// Implements core ABI functions expected
// to be implemented by plugins, for the user.
// Also provides a helper global plugin API instance.

#include <PluginAbi.h>
#include <PluginAPI.h>

#include <cstring>

// this isn't the standard library so it's OK
using namespace collabvm::plugin;

static IPluginApi pluginApiInstance;

/**
 * The global instance of the CollabVM Plugin API.
 */
IPluginApi* g_PluginApi = &pluginApiInstance;

// Exported CollabVM C Plugin ABI functions
extern "C" {

COLLABVM_PLUGINABI_EXPORT int collabvm_plugin_abi_version() {
	return collabvm::plugin::PLUGIN_ABI_VERSION;
}

// TODO: ABI symbol for Boost.ASIO headers

COLLABVM_PLUGINABI_EXPORT void collabvm_plugin_init_api(IPluginApi* apiPointer) {
	// Copy the API data CollabVM gave us into our instance.
	memcpy(&pluginApiInstance, apiPointer, sizeof(IPluginApi));
}
}