#include <boost/program_options.hpp>
#include <boost/version.hpp>

#include <iostream>
#include "CollabVM.h"

namespace po = boost::program_options;

// Uncomment this to allow Asio to use multi-threading. WARNING: Might be buggy
//#define ENABLE_ASIO_MULTITHREADING

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

/**
 * Helper class for all the arguments
 */
struct Arguments {

	bool Process(int argc, char** argv) {
		po::options_description desc("CollabVM Server - Options");
		po::variables_map vm; // moved these here so we *can* catch missing var errors
		try {
			desc.add_options()
				("listen,l", po::value<std::string>(&listen_address)->default_value("0.0.0.0"), "The address to listen on. Defaults to all interfaces.")
				("port,p", po::value<int>(&port)->required(), "The port to listen on")
				("root,r", po::value<std::string>(&http_dir)->default_value("http"), "The root folder to serve HTTP files from. Currently useless, as there is no HTTP web server (yet!)")
				("version,v", "Display server & library versions")
				("help,h", "Show this help screen");

			po::store(po::parse_command_line(argc, argv, desc), vm);

			if (vm.count("help") || argc == 1) {
				std::cout << desc << "\n";
				std::exit(0);
			}

			if (vm.count("version")) {
				std::cout << "CollabVM Server 1.3.0 - AlphaBetas Edition" << '\n'
					<< "Compiled with Boost " << BOOST_VERSION / 100000 << "." << BOOST_VERSION / 100 % 1000 << '\n'
	#ifdef USE_JPEG
					<< "This server is using JPEG.\n"
	#endif
	#ifdef DEBUG
					<< "This server is a debug build.\n"
	#endif
					<< '\n';

				std::exit(0);
			}

			po::notify(vm);
			return true;
		} catch (boost::program_options::required_option::error& e) {
			std::cerr << "A required parameter was not specified!" << "\n"; // make this less vague
			return false;
		} catch (std::exception& e) {
			std::cerr << e.what() << "\n";
			return false;
		} catch(...) {
			std::cerr << "An unknown error has occurred" << "\n";
			return false;
		}
	}

	const std::string& GetListenAddress() const { 
		return listen_address;
	} 

	const std::string& GetDocRoot() const { 
		return http_dir; 
	} 

	int GetPort() const {
		return port;
	}

private:
	std::string listen_address;
	std::string http_dir;
	int port;
};


int main(int argc, char** argv) {
#if defined(BOOST_MSVC) && defined(_DEBUG)
	{
		int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		flags |= _CRTDBG_LEAK_CHECK_DF;
		_CrtSetDbgFlag(flags);
	}
#endif

	Arguments args;

	if(!args.Process(argc, argv))
		return 1;

	std::cout << "CollabVM Server Started" << std::endl;

	boost::asio::io_context ioc;
	std::shared_ptr<CollabVMServer> server;

        // Set up Ctrl+C handler
	boost::asio::signal_set interruptSignal(ioc, SIGINT, SIGTERM);
	interruptSignal.async_wait([&](boost::system::error_code ec, int sig) {
		std::cout << "\nShutting down..." << std::endl;
		server->Stop();
		ioc.stop();
	});

	IgnorePipe();

	server = std::make_shared<CollabVMServer>(ioc);
	server->Run(args.GetListenAddress(), args.GetPort(), args.GetDocRoot());

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
	ioc.run();

#ifdef ENABLE_ASIO_MULTITHREADING
	// Join ASIO completion handler threads when the server is stopping.
	for(auto& thread : threads)
		thread.join();
#endif

	return 0;
}