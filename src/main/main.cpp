#include <Arguments.h>
#include <iostream>


// Uncomment this to allow Asio to use multi-threading. WARNING: Might be buggy
//#define ENABLE_ASIO_MULTITHREADING

#if 0
void IgnorePipe() {
        // Ignores SIGPIPE to prevent LibVNCClient from crashing on Linux
	#ifndef _WIN32
	struct sigaction pipe {};
	pipe.sa_handler = SIG_IGN;
	pipe.sa_flags = 0;
	if(sigaction(SIGPIPE, &pipe, nullptr) == -1)
		std::cout << "Failed to ignore SIGPIPE. Crashes may occur now\n";
	#endif
}
#endif

int main(int argc, char** argv) {
	// maybe global instance this so things can access it?
	collabvm::main::Arguments args;

	args.Process(argc, argv);

#if 0
	boost::asio::io_context ioc;

        // Set up Ctrl+C handler
	boost::asio::signal_set interruptSignal(ioc, SIGINT, SIGTERM);
	interruptSignal.async_wait([&](boost::system::error_code ec, int sig) {
		std::cout << "\nShutting down..." << std::endl;
		server->Stop();
		ioc.stop();
	});
#endif

	//IgnorePipe();

	// boot new CVM server here

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

	// Run the io_service on the main thread.
	//ioc.run();

#ifdef ENABLE_ASIO_MULTITHREADING
	// Join ASIO completion handler threads when the server is stopping.
	for(auto& thread : threads)
		thread.join();
#endif

	return 0;
}
