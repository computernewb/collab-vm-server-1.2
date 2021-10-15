#include <optional>
#include <vector>
#include <string>


namespace collabvm::util {

	/**
	 * Splits a command line into individual arguments,
	 * (internally) using platform specific functions.
	 *
	 * On POSIX, this function uses wordexp() and gates
	 * away command result injection, to avoid any vulnerabilities.
	 *
	 * On Windows, this function uses CommandLineToArgvW(), internally for a bit
	 * converting the input string to a wide string for the picky Windows API.
	 *
	 * \param[in] command Command line to split.
	 * \returns Optional containing split command line, or empty optional on error.
	 */
	std::optional<std::vector<std::string>> SplitCommandLine(const std::string& command);

} // namespace collabvm::util