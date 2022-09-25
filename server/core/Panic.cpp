
#include <core/Panic.hpp>

#include <spdlog/spdlog.h>

#include <cstdlib>

namespace collab3::core {

	[[noreturn]] void Panic(const std::string_view message) {
		spdlog::error("Service Panic (fatal error): {}", message);
		// FIXME: stack trace
		std::quick_exit(1);
	}

} // namespace collab3::core