//
// CollabVM Plugin API
//
// This file is licensed under the GNU Lesser General Public License Version 3
// for external usage in plugins. Text is provided in LICENSE-PLUGIN.
// If you cannot access it, https://www.gnu.org/licenses/lgpl-3.0.html
//
//
// This file defines the CollabVM Server's plugin API, which
// is passed to plugin clients using the ABI.

#ifndef COLLAB_VM_SERVER_ISERVERPLUGIN_H
#define COLLAB_VM_SERVER_ISERVERPLUGIN_H

// include ABI header if not included before
#include "PluginAbi.h"

// SOME maybeDOs:
// - Better buffer passing between boundaries
//	 (or way of indicating lifetime expectation which is ABI safe, e.g: Copied<T*> or ExpectedToLast<T*>)
// - Better passing of string data.
//	 (length / ptr pair?)

namespace collabvm::plugin {

#ifdef __cpp_char8_t
	// Use C++20 char8_t to indicate UTF-8
	using utf8char = char8_t;
#else
	// use uchar then I guess
	#warning I probably wont be supporting this for long
	using utf8char = unsigned char;
#endif

	struct PluginMetadata {
		// Fully qualified plugin name
		const utf8char* PluginName;

		// Plugin description
		const utf8char* PluginDescription;

		// Plugin author
		const utf8char* PluginAuthor;

		// Plugin license
		const utf8char* PluginLicense;

		// Plugin version
		const utf8char* PluginVersion;
	};

	// Interface for VMControllers which is returned by the plugin API
	struct IVmController {
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVmController, const utf8char*, GetName);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVmController, const utf8char*, GetDescription);

		/**
		 * Inserts a message into this
		 * \param[in] content Message content
		 */
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVmController, void, InsertChatMessage, const utf8char* content);
	};

	// Basic plugin API, consumed for either plugin VM controllers
	// or IServerPlugin's
	struct IPluginApi {
		// Write a log message.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IPluginApi, void, WriteLogMessage, const utf8char* message);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IPluginApi, IVmController*, GetVMControllerById, const utf8char* id);
	};

	// Interface
	struct IServerPlugin {
		// Called by the server when it initalizes this plugin
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IServerPlugin, void, Init);
	};

} // namespace collabvm::plugin

#endif //COLLAB_VM_SERVER_ISERVERPLUGIN_H
