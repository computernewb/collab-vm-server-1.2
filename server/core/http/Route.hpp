// TODO: All interfaces in this file, implement a constexpr parser/generator for route regex strings, ...

#pragma once

#include <ctll/fixed_string.hpp>
#include <ctre.hpp> // FIXME: use ctre-unicode probably?
#include <memory>

namespace collab3::core::http {


	namespace detail {
		/**
		 * Base type for HTTP routes.
		 */
		struct RouteBase {

			virtual ~RouteBase() = default;

			virtual bool Matches(const std::string& path) const noexcept = 0;

			// how to:
			// - communicate response (variant return type where tag types indicate what to do?)
			// - pass request         (...)
			// - support pmfs         (could just have everything be a lambda, and have that call the class member, but Ehh)
			virtual void operator()() const = 0;

		   protected:
			friend struct RouteStorage;
			RouteBase* nextRoute;
		};

		// FIXME: Implement properly.

		/**
		 * A static pool for optimizing route storage.
		 * Is initialized in the application lifecycle once.
		 */
		struct RouteStorage {
			RouteStorage();
			~RouteStorage();

			/**
			 * Allocate a single route in route storage.
			 */
			RouteBase* AllocateRoute(std::size_t size);

		   private:

			constexpr bool Overflow() const {
				return
					poolCur >= poolEnd &&
					poolCur < poolBegin;
			}

			constexpr bool WouldOverflow(std::size_t count) const noexcept {
				return (poolCur + count) >= poolEnd;
			}

			constexpr std::size_t UsedMemory() const noexcept {
				return (poolCur - poolBegin);
			}

			constexpr std::size_t FreeMemory() const noexcept {
				return (poolCur - poolEnd);
			}

			constexpr bool Full() const noexcept {
				return FreeMemory() == 0;
			}

			std::uint8_t* poolBegin{};
			std::uint8_t* poolEnd{};

			std::uint8_t* poolCur{};

		};

		extern RouteStorage* gRouteStorage;

		template<ctll::fixed_string RouteRegex, class Fun>
		struct RouteImpl : public RouteBase {

			constexpr explicit RouteImpl(Fun&& fun)
				: handler(std::move(fun)) {

			}

			bool Matches(const std::string& path) const noexcept override {
				return ctre::match<RouteRegex>(path);
			}

			void operator()() const override {
				handler();
			}

		   private:
			Fun handler;
		};

		/**
		 * Create a route.
		 */
		template<ctll::fixed_string RouteRegex, class Fun>
		std::unique_ptr<RouteBase> MakeRoute(Fun&& handler) {
			return std::make_unique<RouteImpl<RouteRegex, Fun>>(std::move(handler));
		}

	}

	using detail::MakeRoute;
}
