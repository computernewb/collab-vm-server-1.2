//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

#include <core/Panic.hpp>

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <boost/stacktrace.hpp>

namespace collab3::core {

	[[noreturn]] void Panic(std::string_view message, const boost::source_location& loc) {
		spdlog::error("Panic @ {}:{} : {}", loc.file_name(), loc.line(), message);

		spdlog::error("Stack trace:");
		for(const auto& frame : boost::stacktrace::stacktrace()) {
			if(!frame.name().empty())
				if(!frame.source_file().empty())
					spdlog::error("    {} ({:p}), @ {}:{}", frame.name(), frame.address(), frame.source_file(), frame.source_line());
				else
					spdlog::error("    {} ({:p})", frame.name(), frame.address());
			else
				spdlog::error("    {:p}", frame.address());
		}

		std::quick_exit(1);
	}

} // namespace collab3::core