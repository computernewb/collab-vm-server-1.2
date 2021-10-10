#include <boost/program_options.hpp>
#include <boost/version.hpp>
#include <iostream>
#include "CollabVM.h"

#if !defined(_WIN32)
        #ifndef __CYGWIN__
                #include "StackTrace.hpp"
        #endif
#else
//#include <windows.h>
#endif

namespace po = boost::program_options;

// Uncomment this to allow Asio to use multi-threading. WARNING: Might be buggy
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

bool process_arguments(int argc, char** argv, std::int16_t& port, std::string& httpDir) {
        int iport;

        try {
				po::options_description desc("CollabVM Server - Options");
				desc.add_options()
					("port,p", po::value<int>(&iport)->required(), "The port to listen on")
					("root,r", po::value<std::string>(&httpDir), "The root folder to serve HTTP files from (default: http). Currently useless, as there is no HTTP werb server (yet!)")
					("version,v", "Display server & library versions")
					("help,h", "Show this help screen");

				po::variables_map vm;
				po::store(po::parse_command_line(argc, argv, desc), vm);

				if (vm.count("help") || argc == 1) {
					std::cout << desc << "\n";
				}

				if (vm.count("version")) {
					std::cout << "CollabVM Server 1.3.0 - AlphaBetas Edition" << "\n"
									<< "Boost " << BOOST_VERSION / 100000 << "." << BOOST_VERSION / 100 % 1000 << "\n"
									#ifdef USE_JPEG
									<< "This server is using JPEG."
									#endif
									#ifdef DEBUG
									<< "This server is a debug build."
									#endif
									<< std::endl;
				}

				po::notify(vm);
        }


		catch (std::exception& e) {
			std::cerr << e.what() << "\n";
		}

		catch(...) {
			std::cerr << "An unknown error has occurred" << "\n";
		}

		port = iport;
		
		return true;
}

int main(int argc, char** argv) {
#if defined(BOOST_MSVC) && defined(_DEBUG)
        {
                int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
                flags |= _CRTDBG_LEAK_CHECK_DF;
                _CrtSetDbgFlag(flags);
        }
#endif
        std::int16_t port;
        std::string httpDir;

        bool result = process_arguments(argc, argv, port, httpDir);

        if (!result)
                return 1;

        if (!port)
                return 1;

        std::cout << "CollabVM Server Started" << std::endl;

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
        server_->Run(port, httpDir);
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

        return 0;
}