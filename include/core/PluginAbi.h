//
// CollabVM Plugin Supplementary
//
// This file is licensed under the GNU Lesser General Public License Version 3
// for external usage in plugins.
//
//
// Defines and shared support code for a server->plugin (or other way round!) ABI
// for CollabVM plugin interfaces (basically classes with just vtable members).
// Essentially "CollabVM COM", except without a baked in factory pattern or true-baseclass pattern.

// TODO: This file should be installed with the collab-vm-server
// 	in e.g: $PREFIX/include/collab-vm/

#ifndef COLLAB_VM_SERVER_PLUGINABI_H
#define COLLAB_VM_SERVER_PLUGINABI_H

// Make a detail name of a vtfunc.
// Do not use this to call a vtfunc,
// use the wrapper COLLABVM_PLUGINABI_DEFINE_VTFUNC adds to the interface.
#define _COLLABVM_PLUGINABI_NAME_VTFUNC(Name) \
	Name##__vtbl_pointer_do_not_use_by_hand

// Define a vtfunc, and a C++-side wrapper which calls
// the given vtfunc automatically.
#define COLLABVM_PLUGINABI_DEFINE_VTFUNC(This, Retty, name, args...)                  \
	Retty (*_COLLABVM_PLUGINABI_NAME_VTFUNC(name))(This * _this, ##args);             \
	template<class... Args>                                                           \
	constexpr Retty name(Args&&... a) {                                               \
		return _COLLABVM_PLUGINABI_NAME_VTFUNC(name)(this, std::forward<Args>(a)...); \
	}

// Assign a vtfunc. Should be used to assign functions to an interface's vtfuncs
// in the constructor of an implementing class.
#define COLLABVM_PLUGINABI_ASSIGN_VTFUNC(name, pointer) \
	_COLLABVM_PLUGINABI_NAME_VTFUNC(name) = collabvm::core::detail::_vtfunc_cast<decltype(_COLLABVM_PLUGINABI_NAME_VTFUNC(name))>(pointer)

// Expected ABI C exports for a CollabVM 3.0 server plugin:
// int collabvm_plugin_abi_version() - returns ABI version. If not matching the server's PluginAbi header, plugin is unloaded.

#ifdef _WIN32
	#define COLLABVM_PLUGINABI_EXPORT _declspec(dllexport)
#else
	#define COLLABVM_PLUGINABI_EXPORT __attribute__((visibility("default")))
#endif

namespace collabvm::core {

	/**
	 * The ABI version of this header.
	 *
	 * Bump if you're going to make incompatible changes to the plugin ABI
	 * which will break existing plugins.
	 */
	constexpr static auto PLUGIN_ABI_VERSION = 0;

	namespace detail {
		/**
		 * Perform a UB cast for ABI vtable function pointers.
		 */
		template<class Dest, class Src>
		constexpr Dest _vtfunc_cast(Src src) {
			union {
				Src src;
				Dest dest;
			} cast_union { .src = src };

			return cast_union.dest;
		}
	} // namespace detail

} // namespace collabvm::core

#endif //COLLAB_VM_SERVER_PLUGINABI_H
