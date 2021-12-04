
#ifndef _WIN32
	#include <csignal>
	#include <iostream>
#endif

namespace collabvm::core {

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

}