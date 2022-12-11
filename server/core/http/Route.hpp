//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

// TODO: All interfaces in this file, implement a constexpr parser/generator for route regex strings, ...

#pragma once

#include <ctll/fixed_string.hpp>
#include <ctre.hpp> // FIXME: use ctre-unicode probably?
#include <memory>

#include <core/asio/AsioConfig.hpp>

namespace collab3::core::http {

	struct Request;
	struct Response;

	namespace detail {

		//
		// HTTP template parameters look like:
		//
		// {<type>}
		// <type> can be, and is passed to your callable ala:
		//		int : int
		//		str : string
		//
		// An example valid template url is something like
		//	"/api/v0/vm/{str}"
		//
		// SlugToTypeList returns a type-list which can later be used to double-check
		//	that a provided callable CAN actually be called with those arguments. if it can't, that's
		//	a compile-time error (on behalf of who's ever trying to do that.).
		//
		// SlugToRegex returns a ctll::fixed_string which itself will match all the slug parameters.
		//



		/**
		 * Just so that things don't need to be function templates
		 * just to work with routes.
		 */
		struct RouteBase {
			virtual ~RouteBase();

			virtual bool Matches() const noexcept = 0;

			virtual Awaitable<void> operator()(std::shared_ptr<Request>, std::shared_ptr<Response>) noexcept = 0;
		};

		// TODO: I don't know if it's really worth it to do this at compile time
		// 	maybe should suck it up and use a trie @ runtime


		template<ctll::fixed_string RouteSlug, class Fun>
		struct Route {

			constexpr explicit Route(Fun&& fun)
				: handler(std::move(fun)) {

			}

			[[nodiscard]] bool Matches(const std::string& path) const noexcept {
				//return ctre::match<RouteSlug>(path);
			}

			Awaitable<void> operator()(std::shared_ptr<Request>, std::shared_ptr<Response>) noexcept {
				// N.B: it's probably a good idea we expect this
				co_await handler();
			}

		   private:
			Fun handler;
		};

		/**
		 * Create a route.
		 */
		template<ctll::fixed_string RouteRegex, class Fun>
		Route<RouteRegex, Fun> MakeRoute(Fun&& handler) {
			return {
				std::forward<Fun>(handler)
			};
		}

	}

	using detail::MakeRoute;
}
