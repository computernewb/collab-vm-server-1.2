//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

#pragma once

//#include <source_location>
#include <string_view>


namespace collab3::core {

	/**
	 * Panic (exit) application on unrecoverable errors.
	 */
	[[noreturn]] void Panic(const std::string_view message);

	#define COLLAB3_VERIFY(expr) if(!(expr)) collab3::core::Panic("COLLAB3_VERIFY(" #expr ") failed.")

#ifndef NDEBUG
	#define COLLAB3_ASSERT(expr) if(!(expr)) collab3::core::Panic("COLLAB3_ASSERT(" #expr ") failed.")
#else
	#define COLLAB3_ASSERT(expr)
#endif

}

