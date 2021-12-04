// Defines and shared support code
// for a server->plugin (or other way round!) ABI for interfaces.

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
// during the construction of an implementing interface.
#define COLLABVM_PLUGINABI_ASSIGN_VTFUNC(name, pointer) _COLLABVM_PLUGINABI_NAME_VTFUNC(name) = collabvm::core::detail::_vtfunc_cast<decltype(_COLLABVM_PLUGINABI_NAME_VTFUNC(name))>(pointer)

namespace collabvm::core {

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
