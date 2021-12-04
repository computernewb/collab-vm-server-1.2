// Defines and shared support code
// for a server->plugin (or other way round!) ABI for virtual
// interfaces. Essentially "CollabVM COM", except without
// a baked in factory pattern or true-baseclass.

#ifndef COLLAB_VM_SERVER_PLUGINABI_H
#define COLLAB_VM_SERVER_PLUGINABI_H

// Get the detail name of a vtable function.
// For introspection purposes only.
#define _COLLABVM_PLUGINABI_NAME_VTFUNC(Name) \
	Name##__vtbl_pointer

#define COLLABVM_PLUGINABI_DEFINE_VTFUNC(This, Retty, name, args...)                  \
	Retty (*_COLLABVM_PLUGINABI_NAME_VTFUNC(name))(This * _this, ##args);             \
	template<class... Args>                                                           \
	constexpr Retty name(Args&&... a) {                                               \
		return _COLLABVM_PLUGINABI_NAME_VTFUNC(name)(this, std::forward<Args>(a)...); \
	}

// Assign a vtable function. Should be used to assign functions
// in the constructor of an implementing interface.
#define COLLABVM_PLUGINABI_ASSIGN_VTFUNC(name, pointer) \
	_COLLABVM_PLUGINABI_NAME_VTFUNC(name) = collabvm::core::detail::_vtfunc_cast<decltype(_COLLABVM_PLUGINABI_NAME_VTFUNC(name))>(pointer)

// Expected ABI C exports for a CollabVM 3.0 server plugin:
// int collabvm_plugin_abi_version() - returns ABI version. If not matching the server's PluginAbi header, plugin is unloaded.

namespace collabvm::core {

	/**
	 * The ABI version of this header.
	 *
	 * Bump if you're going to make incompatible changes to the plugin ABI
	 * which will break existing clients.
	 */
	constexpr static auto PLUGIN_ABI_VERSION = 0;

	namespace detail {
		/**
		 * Perform a UB cast for ABI vtable function pointers.
		 */
		template<class Dest, class Src>
		constexpr Dest _vtfunc_cast(Src s) {
			union {
				Src s;
				Dest d;
			} a { .s = s };

			return a.d;
		}
	} // namespace detail

} // namespace collabvm::core

#endif //COLLAB_VM_SERVER_PLUGINABI_H
