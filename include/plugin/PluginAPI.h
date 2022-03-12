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
#include "DatabasePlugin.h"
#include <new>

// SOME maybeDOs:
// - Better passing of.. well, anything, between plugin/server boundaries.
//	 (or way of indicating lifetime expectation which is ABI safe, e.g: Copied<T*> or ExpectedToLast<T*>)
// - Better passing of string data.
//	 (length / ptr pair?)
// - Pointers shouldn't mean "optional values", we should use STL vocab types for this
//	(e.g: eastl::optional<T>, or if optional isn't abi safe enough, our own abstraction which explicitly converts)
//
// NB: We can use our pinned EASTL version for all of these.
//
// Real TODO's:
//	plugin(or core?)::Signal<R(Args...)>. 
//		Connecting a function to a signal means on signalObj(args...), that all of the connected functions will fire in order.
//		For vocabulary reasons, Signal::operator() == Signal::emit(), and emit() can be used if so desired to spell it out.
//
//	We can use this in IPluginAPI to provide optional signals that plugins
//	can listen for to implement whatever they want.
//
//	Techinically we could also use vtfuncs which are spelled out to only be filled by the plugin, but:
//		- it's more shoddy to do that
//		- it's way less typesafe
//		- that assumes a this/context pointer (which may not be desired)
//		- this only allows one listener, rather than an arbitrary amount.
//
//	Most important of all, it's not elegant like providing a Signal type is.
//	So signals it is.

namespace collabvm::plugin {


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

	/**
	 * Interface for CollabVM users.
	 */
	struct IUser {
		// TODO for roles flesh this out
		enum class Type : std::uint8_t {
			Unregistered,
			Registered,
			Moderator,
			Admin
		};

		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUser, IUserEntry*, GetUserEntry);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUser, IUser::Type, GetType);

		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUser, void, Kick, const utf8char* reason /* Optional, pass empty literal for "no reason". */);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUser, void, Ban, bool isForever, const utf8char* reason /* Optional, pass empty literal for "no reason". */);
	};

	/**
	 * Basic plugin interface.
	 *
	 * Consumed by coreplugins or regular plugins
	 */
	struct IPluginApi {

		enum class LogLevel {
			Info,
			Warning,
			Error
		};

		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IPluginApi, void, WriteLogMessage, IPluginApi::LogLevel level, const utf8char* format, ...);


		// malloc()/free() which goes into the main CollabVM Server heap.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IPluginApi, void*, Malloc, std::size_t size);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IPluginApi, void, Free, void* ptr);

		// TODO: more vtfuncs, for e.g: new way of handling usermessages (by doing serialization in-plugin)
		

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
	};

	/**
	 * Interface for standard CollabVM server plugins.
	 */
	struct IServerPlugin {
		// Called by the server to initialize this plugin.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IServerPlugin, void, Init);

		// TODO: Signals for other events.
	};

} // namespace collabvm::plugin

#endif //COLLAB_VM_SERVER_ISERVERPLUGIN_H
