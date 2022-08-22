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

#include <server/core/config/ConfigStore.h>

int main(int argc, char** argv) {
	// maybe global instance this so things can access it?
	//collab3::host::Arguments args;
	//boost::asio::io_context ioc;

	collab3::core::ConfigStore store;

	store.AddValue("test", "Does this work?");
	store.AddValue("test2", true);
	store.AddValue("test3", static_cast<std::int64_t>(120));
	store.AddValue("test4", static_cast<std::uint64_t>(120));

	std::cout << "test: \"" << store["test"].As<std::string>() << "\"\n";
	std::cout << "test2: \"" << store["test2"].As<bool>() << "\"\n";
	std::cout << "test3: \"" << store["test3"].As<std::int64_t>() << "\"\n";
	std::cout << "test4: \"" << store["test4"].As<std::uint64_t>() << "\"\n";

#if 0
	args.Process(argc, argv);

	// Set up Ctrl+C handler
	boost::asio::signal_set interruptSignal(ioc, SIGINT, SIGTERM);
	interruptSignal.async_wait([&](const boost::system::error_code& ec, int sig) {
		std::cout << "Stopping...\n";
		ioc.stop();
	});


	// Run the io_service on the api thread.
	ioc.run();

#endif
	return 0;
}
