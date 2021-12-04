#include <optional>
#include <vector>
#include <string>


namespace collabvm::core {

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

} // namespace collabvm::util