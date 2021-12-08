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
#include <new>

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
		// Plugin ID. This must NOT conflict.
		const utf8char* PluginId;

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
		 * Inserts a message into this VM controll
		 * \param[in] content Message content
		 */
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IVmController, void, InsertChatMessage, const utf8char* content);
	};

	/**
	 * Basic plugin interface.
	 *
	 * Consumed by coreplugins or regular plugins
	 */
	struct IPluginApi {
		// Write a log message.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IPluginApi, void, WriteLogMessage, const utf8char* message);

		// malloc()/free() which goes into the main CollabVM Server heap.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IPluginApi, void*, Malloc, std::size_t size);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IPluginApi, void, Free, void* ptr);

		// a shoddy hack because global operators do nothing.
		// Only use these on the plugin side, please.

		template<class T, class ...Args>
		inline T* New(Args&&... args) {
			return new (Malloc(sizeof(T))) T(std::forward<Args>(args)...);
		}

		template<class T>
		inline void Delete(T* ptr) {
			if(ptr) {
				ptr->~T();
				Free(ptr);
			}
		}

		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IPluginApi, IVmController*, GetVMControllerById, const utf8char* id);
	};

	struct IUser {
		using snowflake_t = std::uint64_t;

		// Get this user's ID/snowflake.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUser, snowflake_t, GetSnowflake);

		// Get username.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUser, const utf8char*, GetUsername);

	};

	/**
	 * Interface for standard plugins.
	 */
	struct IServerPlugin {
		// Called by the server when it initalizes this plugin
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IServerPlugin, void, Init);

		// IUser* LookupUser()
		//
	};

} // namespace collabvm::plugin

#endif //COLLAB_VM_SERVER_ISERVERPLUGIN_H
