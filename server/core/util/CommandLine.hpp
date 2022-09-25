//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

// these two files will probably be dropped soon, or something

#include <optional>
#include <string>
#include <vector>

namespace collab3::core {

	/**
	 * Splits a command line into individual arguments,
	 * providing a clean layer over platform specific functions.
	 *
	 * On POSIX, this function uses wordexp() and gates
	 * away command result injection, to avoid any vulnerabilities.
	 *
	 * On Windows, this function uses CommandLineToArgvW().
	 *
	 * \param[in] command Command line to split.
	 * \returns Optional containing split command line, or empty optional upon error.
	 */
	std::optional<std::vector<std::string>> SplitCommandLine(const std::string& command);

} // namespace collab3::core