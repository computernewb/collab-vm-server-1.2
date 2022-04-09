//
// CollabVM Server
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#ifndef _WIN32
	#include <csignal>
	#include <iostream>
#endif

namespace collab3::core {

	void IgnoreSigPipe() {
#ifndef _WIN32
		struct sigaction pipe {
			.sa_flags = 0
		};

		// this cannot be initalized named because
		// signal.h sucks
		pipe.sa_handler = SIG_IGN;

		if(sigaction(SIGPIPE, &pipe, nullptr) == -1)
			std::cout << "Failed to ignore SIGPIPE.\n";
#endif
	}

} // namespace collab3::core