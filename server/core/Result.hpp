//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

#pragma once

#include <tl/expected.hpp>

namespace collab3::core {

	template<class T, class E>
	using Result = tl::expected<T, E>;

}