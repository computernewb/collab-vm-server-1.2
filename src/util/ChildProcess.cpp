#include <util/ChildProcess.h>
#include <util/CommandLine.h>


#include <boost/process/args.hpp>
#include <boost/process/async.hpp>
#include <boost/process/search_path.hpp>


namespace collabvm::util {

	// helper literally banking on c++17 onwards RVO lmao
	template<class... Attrs>
	inline boost::process::child SpawnChild(Attrs&&... attrs) {
		return boost::process::child{std::forward<Attrs>(attrs)...};
	}

	boost::process::child StartChildProcess(boost::asio::io_context& ioc, const std::string& commandline, OnExitHandler&& onexit) {
		auto split = SplitCommandLine(commandline);

		// TODO: probably should find some way to get mad if the split command line fails to work

		return StartChildProcess(ioc, split.value(), std::move(onexit));
	}

	boost::process::child StartChildProcess(boost::asio::io_context& ioc, const std::vector<std::string>& args, OnExitHandler&& onexit) {
		if(args.size() < 2)		
			return SpawnChild(boost::process::search_path(args[0]), boost::process::on_exit = onexit, ioc);
		else {
			auto argsified = std::vector<std::string>{args.begin() + 1, args.end()};
			return SpawnChild(boost::process::search_path(args[0]), boost::process::args = argsified, boost::process::on_exit = std::move(onexit), ioc);
		}
	}	

} // namespace collabvm::util