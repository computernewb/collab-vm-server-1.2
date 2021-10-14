#include <util/CommandLine.h>

#include <cstring>

// (unfortunately) include platform specific headers
#ifndef _WIN32
	#include <wordexp.h>
#else
	#define _WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <shellapi.h>
#endif

namespace collabvm::util {

	namespace {
		bool SplitCommandLineImpl(std::vector<std::string>& split_line, const std::string& command) {
#ifndef _WIN32
			// POSIX wordexp() implementation

			wordexp_t exp;

			// Do the wordexp(), not allowing command result injection.
			// Command result injection is a security vulnerability,
			// and I honest to god wonder how/why no one caught this before.
			if(wordexp(command.c_str(), &exp, WRDE_NOCMD))
				return false;

			split_line.reserve(exp.we_wordc);

			for(int i = 0; i < exp.we_wordc; i++) {
#ifdef DEBUG
				// I don't know if this is an actual possibility or not.
				// if it's not this code could safely be removed
				if(!exp.we_wordv[i])
					continue;
#endif

				const int len = std::strlen(exp.we_wordv[i]) + 1;
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
			wchar_t** wargv = nullptr;
			LPCWSTR cmdLineW;
			cmdLineW = GetCommandLineW();
			LPWSTR* wargs;
			wargs = CommandLineToArgvW(cmdLineW, &argc); // What the hell is cmdLineW meant to be?

			std::wstring wide_cmdline;
			wide_cmdline.resize(command.size());

			if(!MultiByteToWideChar(CP_ACP, 0, command.c_str(), -1, wide_cmdline.data(), command.size()))
				return false;

			if(!(wargs)) // You can't just specify an identifier and expect it to work like that.
				return false;

			split_line.reserve(argc);

			// Convert from a wide string back to MBCS
			// and then put it in our split vector.
			for(int i = 0; i < argc; i++) {
				// Get the size needed for the target buffer.
				const size_t targetBufLen = WideCharToMultiByte(CP_ACP, 0, wargs[i], -1, NULL, 0, NULL, NULL);
				std::string str;

				str.resize(targetBufLen);

				// Do the conversion.
				WideCharToMultiByte(CP_ACP, 0, wargs[i], -1, str.data(), targetBufLen, NULL, NULL);

				split_line.push_back(str);
			}

			// the wide argv is no longer needed
			if(wargv)
				LocalFree(wargv);
#endif // WIN32

			return true;
		}
	}

	std::optional<std::vector<std::string>> SplitCommandLine(const std::string& command) {
		std::vector<std::string> split_line;

		if(!SplitCommandLineImpl(split_line, command))
			return std::nullopt;

		return split_line;
	}

} // namespace collabvm::util