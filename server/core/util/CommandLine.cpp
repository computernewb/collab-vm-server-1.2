//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#include "CommandLine.hpp"

#include <cstring>

// (unfortunately) include platform specific headers
#ifndef _WIN32
	#include <wordexp.h>
#else
	#define _WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

namespace collab3::core {

	namespace {
		bool SplitCommandLineImpl(std::vector<std::string>& split_line, const std::string& command) {
#ifndef _WIN32
			// POSIX implmentation, using wordexp()

			wordexp_t exp;

			// Do the wordexp(), not allowing command result injection.
			// Command result injection is a security vulnerability,
			// and I honest to god wonder how/why no one caught this before.
			if(wordexp(command.c_str(), &exp, WRDE_NOCMD))
				return false;

			split_line.reserve(exp.we_wordc);

			for(int i = 0; i < exp.we_wordc; i++) {
				const auto len = std::strlen(exp.we_wordv[i]) + 1;

				// won't this always be true?
				// ah whatever. cosmic code.
				if(!len)
					continue;

				// Insert the string into the split line,
				// using the explicit (CharT*, size_t) constructor for std::string.
				split_line.emplace_back(exp.we_wordv[i], len);
			}

			wordfree(&exp);
#else
			// Windows has CommandLineToArgvW but not CommandLineToArgvA
			// so the command line must be converted to unicode first.
			// This kind of sucks, tbh.

			int argc;
			wchar_t** argv = nullptr;

			std::wstring wide_cmdline;
			wide_cmdline.resize(command.size());

			if(!MultiByteToWideChar(CP_ACP, 0, command.c_str(), -1, wide_cmdline.data(), command.size()))
				return false;

			if(!(argv = CommandLineToArgvW(wide_cmdline.data(), &argc)))
				return false;

			split_line.reserve(argc);

			// Convert from a wide string back to MBCS
			// and then put it in our split vector.
			for(int i = 0; i < argc; i++) {
				// Get the size needed for the target buffer.
				const size_t targetBufLen = WideCharToMultiByte(CP_ACP, 0, argv[i], -1, NULL, 0, NULL, NULL);
				std::string str;

				str.resize(targetBufLen);

				// Do the conversion.
				WideCharToMultiByte(CP_ACP, 0, argv[i], -1, str.data(), targetBufLen, NULL, NULL);

				split_line.push_back(str);
			}

			// the wide argv is no longer needed
			if(argv)
				LocalFree(argv);
#endif // _WIN32

			return true;
		}
	} // namespace

	std::optional<std::vector<std::string>> SplitCommandLine(const std::string& command) {
		std::vector<std::string> split_line;

		if(!SplitCommandLineImpl(split_line, command))
			return std::nullopt;

		return split_line;
	}

} // namespace collab3::core