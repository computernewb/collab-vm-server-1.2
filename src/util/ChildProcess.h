#ifndef UTIL_CHILDPROCESS_H
#define UTIL_CHILDPROCESS_H

#include <string>
#include <functional>

#include <boost/asio/io_context.hpp>

#include <boost/process/child.hpp>


namespace collabvm::util {

	/**
	 * type for the on-exit handler
	 */
	using OnExitHandler = std::function<void(int /* exitcode */, const std::error_code& /* ec */)>;

	/**
	 * Start a child process, with a commandline. Will internally split the command line
	 */
	boost::process::child StartChildProcess(boost::asio::io_context& ioc, const std::string& commandline, OnExitHandler&& onexit);	

	/**
	 * Start a child process with a pre-split command line.
	 */
	boost::process::child StartChildProcess(boost::asio::io_context& ioc, const std::vector<std::string>& args, OnExitHandler&& onexit);

	// TODO: TimedStop?

} // namespace collabvm::util

#endif // UTIL_CHILDPROCESS_H