#include <Arguments.h>

#include <boost/program_options.hpp>
#include <boost/version.hpp>

#include <cstdlib>

#include <iostream>

namespace po = boost::program_options;

namespace collabvm::main {

	void Arguments::Process(int argc, char** argv) {
		po::options_description desc("CollabVM Server - Options");
		po::variables_map vm; // moved these here so we *can* catch missing var errors
		try {
			// clang-format off
			desc.add_options()
				("listen,l", po::value<std::string>(&listen_address)->default_value("0.0.0.0"), "The address to listen on. Defaults to all interfaces.")
				("port,p", po::value<int>(&port)->required(), "The port to listen on")
				("root,r", po::value<std::string>(&http_dir)->default_value("http"), "The root folder to serve HTTP files from. Currently useless, as there is no HTTP web server (yet!)")
				("version,v", "Display server & library versions")
				("help,h", "Show this help screen");

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
				// FIXME: This version should probably be derived by something so we don't need to hand-twiddle it.
				std::cout << "CollabVM Server 3.0.0" BUILDSUFFIX_CAT " - AlphaBetas Edition\n\n"
						  << "Compiled with:\n"
						  << "\t\b\b\b\b- Boost " << BOOST_VERSION / 100000 << "." << BOOST_VERSION / 100 % 1000 << '\n'
						  << '\n';

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
			std::cerr << "An unknown error has occurred"
					  << "\n";
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

} // namespace collabvm::main