//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#include "Arguments.h"

#include <boost/program_options.hpp>
#include <cstdlib>
#include <iostream>

// Version headers
#include <server/core/Version.h>

#include <boost/asio/version.hpp>
#include <boost/beast/version.hpp>
#include <boost/version.hpp>

namespace po = boost::program_options;

namespace collab3::host {

	void Arguments::Process(int argc, char** argv) {
		po::options_description desc("Collab3 Backend - Options");
		po::variables_map vm;
		try {
			// clang-format off
			desc.add_options()
				("listen,l", po::value<std::string>(&listen_address)->default_value("0.0.0.0"), "The address to listen on HTTP and WebSockets. Defaults to all interfaces.")
				("port,p", po::value<int>(&port)->required(), "The port to listen on.")
				("proxy,P", "Enable load balancing proxy support.")
				("version,V", "Display server, library, and internal versions")
				("verbose,v", "Enable verbose log messages (for debugging purposes)")
				("help,h", "Show this help");

			// clang-format on

			po::store(po::parse_command_line(argc, argv, desc), vm);

			if(vm.count("help") || argc == 1) {
				std::cout << desc << "\n";
				std::exit(0);
			}

#ifdef COLLABVM_CORE_DEBUG
	#define BUILDSUFFIX_CAT " (Debug)"
#else
	#define BUILDSUFFIX_CAT ""
#endif

			if(vm.count("version")) {
				std::cout << "Collab3 Backend" << version::tag << BUILDSUFFIX_CAT " - AlphaBetas Edition \n\n"
						  << "Compiled with:\n"
						  // library versions
						  << "\t\b\b\b\b- Boost " << BOOST_VERSION / 100000 << '.' << BOOST_VERSION / 100 % 1000 << '\n'
						  << "\t\b\b\b\b- Boost.Asio " << BOOST_ASIO_VERSION / 100000 << '.'
						  << BOOST_ASIO_VERSION / 100 % 1000 << '.' << BOOST_ASIO_VERSION % 100 << '\n'
						  << "\t\b\b\b\b- Boost.Beast v" << BOOST_BEAST_VERSION << '\n';
				// internal versions
				std::exit(0);
			}

			po::notify(vm);
		} catch(boost::program_options::required_option::error& e) {
			std::cerr << "Error: " << e.what() << '\n';
			std::exit(1);
		} catch(std::exception& e) {
			std::cerr << e.what() << "\n";
			std::exit(1);
		} catch(...) {
			std::cerr << "An unknown error has occurred\n";
			std::exit(1);
		}
	}

	const std::string& Arguments::GetListenAddress() const {
		return listen_address;
	}

	const std::string& Arguments::GetDocRoot() const {
		return http_dir;
	}

	int Arguments::GetPort() const {
		return port;
	}

} // namespace collab3::api