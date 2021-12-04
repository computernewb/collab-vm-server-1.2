#ifndef COLLABVM_MAIN_ARGUMENTS_H
#define COLLABVM_MAIN_ARGUMENTS_H

#include <string>

namespace collabvm::main {

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

} // namespace collabvm::main

#endif // COLLABVM_MAIN_ARGUMENTS_H