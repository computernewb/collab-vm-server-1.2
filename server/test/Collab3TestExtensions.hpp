//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

// Test extensions for using tl::expected/core::Result
// basically the same as exception test macros in Catch.

#pragma once

/**
 * Require that the given expression returns an expected in the error state.
 * It does not matter what error code is inside of the expected, only
 * that it is indeed in an error state.
 * (FIXME?: this should not pass for "empty" monostate expecteds)
 */
#define COLLAB3_REQUIRE_ERROR(expr)       \
	do {                                  \
		auto expr_value = expr;           \
		REQUIRE(!expr_value.has_value()); \
	} while(0)

/**
 * Require that the given expression returns an expected in the expected state.
 */
#define COLLAB3_REQUIRE_VALUE(expr)      \
	do {                                 \
		auto expr_value = expr;          \
		REQUIRE(expr_value.has_value()); \
	} while(0)

/**
 * Require that the given expression returns an expected in the error state,
 * with the given error code.
 */
#define COLLAB3_REQUIRE_ERROR_AS(expr, error_) \
	do {                                       \
		auto expr_value = expr;                \
		REQUIRE(!expr_value.has_value());      \
		REQUIRE(expr_value.error() == error_); \
	} while(0)
