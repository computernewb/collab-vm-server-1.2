//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

#pragma once

/**
 * Require that the given expression returns an expected in the error
 * state.
 */
#define REQUIRE_ERROR(expr)               \
	do {                                  \
		auto expr_value = expr;           \
		REQUIRE(!expr_value.has_value()); \
	} while(0)

/**
 * Require that the given expression returns an expected in the error state,
 * with the given error code.
 */
#define REQUIRE_ERROR_AS(expr, error_)                                    \
	do {                                                                  \
		auto expr_value = expr;                                           \
		REQUIRE(!expr_value.has_value() && expr_value.error() == error_); \
	} while(0)