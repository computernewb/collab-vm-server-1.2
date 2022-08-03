//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#ifndef HOST_ARGUMENTS_H
#define HOST_ARGUMENTS_H

#include <string>

namespace collab3::host {

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

} // namespace collab3::api

#endif // API_ARGUMENTS_H