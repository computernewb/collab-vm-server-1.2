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
// - Better buffer passing between boundaries
//	 (or way of indicating lifetime expectation which is ABI safe, e.g: Copied<T*> or ExpectedToLast<T*>)
// - Better passing of string data.
//	 (length / ptr pair?)

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
		enum class Type : std::uint8_t {
			Unregistered,
			Registered,
			Moderator,
			Admin
		};

		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUser, IUserEntry*, GetUserEntry);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUser, IUser::Type, GetType);

		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUser, void, Kick, const utf8char* reason /* Optional, pass nullptr for "no reason" */);
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IUser, void, Ban, bool isForever, const utf8char* reason /* Optional, pass nullptr for "no reason" */);
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

		// websocket stuff

		/**
		 * Register a usermessage with the given ID and handler.
		 * the server will dispatch to this plugin automatically.
		 *
		 * \param[in] id The user-message ID.
		 * \param[in] handler The user-message handler.
		 */
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IPluginApi, std::uint32_t /* "Handle */, RegisterUserMessage, std::uint32_t id, void(*handler)(IPluginApi* plugin, const std::uint8_t* buffer, std::size_t length));

		/**
		 * Unregister a previously registered (by IPluginApi::RegisterUserMessage()) plugin usermessage.
		 */
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IPluginApi, void, UnregisterUserMessage, std::uint32_t handle);

		/**
		 * Send a message to an individual CollabVM user.
		 */
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IPluginApi, void, SendMessage, IUser* user, std::uint8_t* buffer, std::size_t buffer_length);

		/**
		 * Broadcast a message to all CollabVM users.
		 */
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IPluginApi, void, BroadcastMessage, std::uint8_t* buffer, std::size_t buffer_length);

		// TODO: more vtfuncs

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
		// Called by the server when it initializes this plugin.
		COLLABVM_PLUGINABI_DEFINE_VTFUNC(IServerPlugin, void, Init);
	};

} // namespace collabvm::plugin

#endif //COLLAB_VM_SERVER_ISERVERPLUGIN_H
