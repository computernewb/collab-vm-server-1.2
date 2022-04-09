//
// CollabVM Server
//
// (C) 2021-${YEAR} CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>

#include "Arguments.h"

//#include <core/Server.h>

#include "core/PluginManager.h"

// Uncomment this to allow Asio to use multi-threading. WARNING: Might be buggy
//#define ENABLE_ASIO_MULTITHREADING

int main(int argc, char** argv) {
	// maybe global instance this so things can access it?
	collab3::main::Arguments args;
	boost::asio::io_context ioc;

	args.Process(argc, argv);

	// Set up Ctrl+C handler
	boost::asio::signal_set interruptSignal(ioc, SIGINT, SIGTERM);
	interruptSignal.async_wait([&](const boost::system::error_code& ec, int sig) {
		std::cout << "Stopping...\n";
		ioc.stop();
	});

	// FIXME: If this is re-enabled, maybe default to (nproc() / 2) -1,
	// 	but add a --threads command-line argument?
#ifdef ENABLE_ASIO_MULTITHREADING
	std::vector<std::thread> threads;

	const auto N = (std::thread::hardware_concurrency() / 2) - 1;
	threads.reserve(N);

	// Notify user how many threads the server will spawn to run completion handlers
	std::cout << "Running server ASIO completion handlers on " << N << " worker threads\n";
	std::cout << "Your system will actually run " << N + 1 << " worker threads including main thread\n";

	for(int j = 0; j < N; ++j) {
		threads.emplace_back([&ioc]() {
			ioc.run();
		});
	}
#endif

	// Test plugin hosting using core::PluginManager API
	collab3::core::PluginManager pm;
	if(!pm.Init()) {
		std::cout << "Uh-oh!\n";
	}

	// Run the io_service on the main thread.
	ioc.run();

#ifdef ENABLE_ASIO_MULTITHREADING
	// Join ASIO completion handler threads when the server is stopping.
	for(auto& thread : threads)
		thread.join();
#endif

	return 0;
}
