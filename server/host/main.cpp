//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

// Simple host

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>

#include "Arguments.h"

int main(int argc, char** argv) {
	// maybe global instance this so things can access it?
	collab3::host::Arguments args;
	boost::asio::io_context ioc;

	args.Process(argc, argv);

	// Set up Ctrl+C handler
	boost::asio::signal_set interruptSignal(ioc, SIGINT, SIGTERM);
	interruptSignal.async_wait([&](const boost::system::error_code& ec, int sig) {
		std::cout << "Stopping...\n";
		ioc.stop();
	});


	// Run the io_service on the api thread.
	ioc.run();


	return 0;
}
