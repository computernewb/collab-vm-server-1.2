//
// CollabVM Plugin Supplementary
//
// This file is licensed under the GNU Lesser General Public License Version 3
// for external usage in plugins. Text is provided in LICENSE-PLUGIN.
// If you cannot access it, https://www.gnu.org/licenses/lgpl-3.0.html
//
//
// An ABI-safe std::function replacement.
// This file is ABI important. Any breaking changes to the Function class,
// or to the BaseInvocable class, warrant an ABI version bump.

#ifndef COLLAB_VM_SERVER_VTFUNCTION_H
#define COLLAB_VM_SERVER_VTFUNCTION_H

#include <new>

#include <cstdio>

namespace collabvm::plugin {

	namespace detail {

		template<class T>
		constexpr T&& Forward(T& t) {
			return static_cast<T&&>(t);
		}

		// Base-class for invocable things.
		template<class T, class... Args>
		struct BaseInvocable {
			virtual ~BaseInvocable() = default;

			// Invocation operator
			virtual T operator()(Args&&...) = 0;
		};

		// A plain function. Nothing special.
		template<class T, class... Args>
		struct PlainFn : public BaseInvocable<T, Args...> {
			using FuncT = T (*)(Args...);

			constexpr explicit PlainFn(FuncT ptr)
				: fp(ptr) {
			}

			T operator()(Args&&... args) override {
				return fp(Forward(args)...);
			}

		   private:
			FuncT fp {};
		};

		// Bound member function.
		// For simplicity, we don't do param binding.
		template<class T, class R, class... Args>
		struct BoundMemFn : public BaseInvocable<R, Args...> {
			using FuncT = R (T::*)(Args...);

			constexpr explicit BoundMemFn(T& t, FuncT ptr)
				: bound_t(t),
				  fp(ptr) {
			}

			R operator()(Args&&... args) override {
				return (bound_t.*(fp))(Forward(args)...);
			}

		   private:
			T& bound_t;
			FuncT fp {};
		};

	} // namespace detail

	template<class T>
	struct VtFunction;

	template<class R, class... Args>
	struct VtFunction<R(Args...)> {

		constexpr VtFunction()
			: using_sso(true) {
			invokee = nullptr;
			std::printf("default!\n");
		}

		// constructor for "bare" functions,
		// or captureless lambda functions
		constexpr VtFunction(typename detail::PlainFn<R, Args...>::FuncT barePtr) noexcept
			: invokee(choose_construct<detail::PlainFn<R, Args...>>(barePtr)) {
		}

		// constructor for binding a member function
		template<class T>
		constexpr VtFunction(T& t, typename detail::BoundMemFn<T, R, Args...>::FuncT fn) noexcept {

			std::printf("MEMFN!!!\n");
			invokee = choose_construct<detail::BoundMemFn<T, R, Args...>>(t, fn);
		}

		// TODO: move and copy.

		~VtFunction() {

			// Don't destroy null invokees
			if(invokee == nullptr)
				return;

			// We only need to call the BaseInvocable
			// d-tor if we're using SSO.
			if(using_sso) {
				std::printf("sso destruct\n");
				invokee->~BaseInvocable();
				return;
			}
			//else
			//	delete invokee;
		}

		constexpr R operator()(Args... args) noexcept {
			return invokee->operator()(detail::Forward(args)...);
		}

		//constexpr R operator()(Args&&... args) noexcept {
		//	if(!invokee)
		//		std::printf("ouch\n");
		//	return invokee->operator()(detail::Forward(args)...);
		//}

	   private:
		// the BaseInvocable<> type for this Function template
		using invoke_t = detail::BaseInvocable<R, Args...>;

		// utilitarian helper to choose to construct either inside of the
		// SSO storage, or allocate + construct on the heap.
		template<class T, class... Args2>
		constexpr T* choose_construct(Args2&&... args) {
			if constexpr(sizeof(T) <= sizeof(_sso_storage)) {

				std::printf("yes sso\n");
				using_sso = true;
				return new(&_sso_storage[0]) T(detail::Forward<Args2>(args)...);
			} else {
				std::printf("no sso\n");
				//using_sso = false;
				//return new T(detail::Forward<Args2>(args)...);
			}
		}

		// If you change this layout, PLEASE bump
		// abi version in PluginAbi.h
		invoke_t* invokee;
		bool using_sso;

		// 4 pointers worth of storage.
		alignas(invoke_t) unsigned char _sso_storage[sizeof(void*) * 4]{0};
	};
} // namespace collabvm::plugin

#endif //COLLAB_VM_SERVER_VTFUNCTION_H
