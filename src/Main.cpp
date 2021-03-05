#include <iostream>
#include "CollabVM.h"

#if !defined(_WIN32)
	#ifndef __CYGWIN__
		#include "StackTrace.hpp"
	#endif
#else
//#include <windows.h>
#endif

// Enable this to allow Asio multi threading
// Currently a bit buggy and unneeded with beast..
//#define ENABLE_ASIO_MULTITHREADING

void IgnorePipe() {
	// Ignores SIGPIPE to prevent LibVNCClient from crashing on Linux
#ifndef _WIN32
	struct sigaction pipe {};
	pipe.sa_handler = SIG_IGN;
	pipe.sa_flags = 0;
	if(sigaction(SIGPIPE, &pipe, nullptr) == -1) {
		std::cout << "Failed to ignore SIGPIPE. Crashies may occur now\n";
	}
#endif
}

#ifndef UNIT_TEST
int main(int argc, char* argv[]) {
	#ifndef _WIN32
	if(!getuid()) std::cout << "\u001b[33mWarning:\u001b[0m CollabVM does not require root to run! Unless you are running a VM which requires administrator privileges, it is not advised to run as root.\n\n";
	if(!fopen("/dev/kvm", "w")) std::cout << "\u001b[33mWarning:\u001b[0m You do not have access to KVM! The server can still run but your VMs will not be accelerated, therefore they will run slowly.\nMake sure you have Virtualization enabled, have access to /dev/kvm, are in the KVM group or your CPU supports VT-x or AMD-V.\n\n";
	#endif
	try {
		if(argc < 2 || argc > 3) {
			std::cout << "Usage: [Port] [HTTP dir]\n";
			std::cout << "Port - the port to listen on for websocket and http requests\n";
			std::cout << "HTTP dir (optional) - the directory to serve HTTP files from. defaults to \"http\""
						 " in the current directory\n";
			std::cout << "\tEx: 80 web" << std::endl;
			return -1;
		}

		std::string s(argv[1]);
		size_t i;
		int port = stoi(s, &i);
		if(i != s.length() || !(port > 0 && port <= UINT16_MAX)
	#ifndef _WIN32
			//|| (port > 1024 && geteuid() != 0)
	#endif
		) {
			std::cout << "Invalid port for WebSocket server." << std::endl;
			return -1;
		}

		std::cout << "Collab VM Server started" << std::endl;

		boost::asio::io_service service_;
		std::shared_ptr<CollabVMServer> server_;

		// Set up Ctrl+C handler
		boost::asio::signal_set interruptSignal(service_, SIGINT, SIGTERM);
		interruptSignal.async_wait([&](boost::system::error_code ec, int sig) {
			std::cout << "\nShutting down..." << std::endl;
			//work.reset();
			server_->Stop();
			service_.stop();
		});

		IgnorePipe();

		server_ = std::make_shared<CollabVMServer>(service_);
		server_->Run(port, argc > 2 ? argv[2] : "http");

		// If the server has multithreading enabled, then
		// spawn the threads now.
	#ifdef ENABLE_ASIO_MULTITHREADING
		std::vector<std::thread> threads;

		const auto N = (std::thread::hardware_concurrency() / 2) - 1;
		threads.reserve(N);

		// Notify user how many threads the server will spawn to run completion handlers
		std::cout << "Running server ASIO completion handlers on " << N << " worker threads\n";
		std::cout << "Your system will actually run " << N + 1 << " worker threads including main thread\n";

		for(int j = 0; j < N; ++j) {
			threads.emplace_back([&service_]() {
				service_.run();
			});
		}
	#endif
		// Run the io_service on the main thread.
		service_.run();

	#ifdef ENABLE_ASIO_MULTITHREADING
		// Join ASIO completion handler threads when the server is stopping.
		for(auto& thread : threads)
			thread.join();
	#endif
	} catch(const std::exception& e) {
		std::cout << "An exception was thrown:" << std::endl;
		std::cout << e.what() << std::endl;
	#if !defined(_WIN32) && !defined(__CYGWIN__)
		PrintStackTrace();
	#endif
		return -1;
	}
	return 0;
}
#endif
