//
// CollabVM Plugin Supplementary
//
// This file is licensed under the GNU Lesser General Public License Version 3
// for external usage in plugins.
//

#include <core/PluginAbi.h>

extern "C" {

COLLABVM_PLUGINABI_EXPORT int collabvm_plugin_abi_version() {
	return collabvm::core::PLUGIN_ABI_VERSION;
}

}