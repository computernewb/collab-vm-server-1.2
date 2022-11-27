//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

#include <core/FundamentalTypes.hpp>
#include <core/Panic.hpp>

#include <core/http/Route.hpp>

namespace collab3::core::http {

	/**
	 * The size of the route pool.
	 */
	constexpr static inline auto RoutePoolSize = 1 * MB;

	// FIXME: Route pool should presumably be in a detail source file?

	namespace detail {

		RouteStorage::RouteStorage() {
			COLLAB3_VERIFY(gRouteStorage == nullptr && "Route pool double-init!");
			gRouteStorage = this;

			poolBegin = new (std::nothrow) std::uint8_t[RoutePoolSize];

			// App bring up would fail, so this stays a verify.
			COLLAB3_VERIFY(poolBegin != nullptr && "Pool memory allocation failed.");

			poolEnd = poolBegin + RoutePoolSize;
			poolCur = poolBegin;
		}

		RouteStorage::~RouteStorage() {
			COLLAB3_ASSERT(gRouteStorage != nullptr && "Route pool not initalized?");
			gRouteStorage = nullptr;
			delete[] poolBegin;
		}

		RouteBase* RouteStorage::AllocateRoute(std::size_t size) {

			// FIXME: This function should throw a Error<>.
			COLLAB3_VERIFY(WouldOverflow(size) && "Allocation would overflow");

			// Note that this VERIFY is perfectly fine.
			COLLAB3_VERIFY(Full() && "Out of route pool (somehow.)");

			auto* prevRoute = reinterpret_cast<RouteBase*>(poolCur);
			auto* nextRoute = reinterpret_cast<RouteBase*>(poolCur + size);

			// Set the previous route's chain pointer to this one
			prevRoute->nextRoute = nextRoute;

			// Return the new route
			poolCur = reinterpret_cast<std::uint8_t*>(nextRoute);
			return nextRoute;
		}


	}

}