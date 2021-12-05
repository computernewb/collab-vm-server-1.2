
#include <core/PluginAbi.h>

extern "C" {

COLLABVM_PLUGINABI_EXPORT int collabvm_plugin_abi_version() {
	return collabvm::core::PLUGIN_ABI_VERSION;
}

}