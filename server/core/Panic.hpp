//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

#pragma once

#include <boost/assert/source_location.hpp>
#include <string_view>
#include <utility>

namespace collab3::core {

	/**
	 * Panic (exit) application on unrecoverable errors.
	 */
	[[noreturn]] void Panic(std::string_view message, boost::source_location const& loc = BOOST_CURRENT_LOCATION);

	// TODO find a better spot for this

#define COLLAB3_VERIFY(expr)                                           \
	do {                                                               \
		if(!(expr)) [[unlikely]] {                                     \
			collab3::core::Panic("COLLAB3_VERIFY(" #expr ") failed."); \
			__builtin_unreachable();                                   \
		}                                                              \
	} while(0)

#ifndef NDEBUG
	#define COLLAB3_ASSERT(expr)                                           \
		do {                                                               \
			if(!(expr)) [[unlikely]] {                                     \
				collab3::core::Panic("COLLAB3_ASSERT(" #expr ") failed."); \
				__builtin_unreachable();                                   \
			}                                                              \
		} while(0)
#else
	#define COLLAB3_ASSERT(expr)
#endif

#define COLLAB3_TRY(expr)                     \
	({                                        \
		auto _result = (expr);                \
		if(!_result.has_value()) [[unlikely]] \
			return std::move(result).error(); \
		return std::move(result).value();     \
	})

#define COLLAB3_MUST(expr)                \
	({                                    \
		auto _result = (expr);            \
		COLLAB3_VERIFY(!expr.has_error()) \
		return std::move(result).value(); \
	})

} // namespace collab3::core
