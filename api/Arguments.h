#ifndef COLLABVM_MAIN_ARGUMENTS_H
#define COLLABVM_MAIN_ARGUMENTS_H

#include <string>

namespace collab3::main {

	// TODO: this class probably won't be needed, either that or adopt a common config store in core

	/**
	 * Helper class for all the arguments
	 */
	struct Arguments {
		void Process(int argc, char** argv);

		const std::string& GetListenAddress() const;
		const std::string& GetDocRoot() const;
		int GetPort() const;

	   private:
		std::string listen_address;
		std::string http_dir;
		int port;
	};

} // namespace collab3::main

#endif // COLLABVM_MAIN_ARGUMENTS_H