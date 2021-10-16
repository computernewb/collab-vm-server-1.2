#ifndef UTIL_CHILDPROCESS_H
#define UTIL_CHILDPROCESS_H

#include <string>
#include <functional>
#include <memory>

#include <boost/asio/io_context.hpp>


namespace collabvm::util {

	/**
	 * Interface for child processes.
	 */
	struct ChildProcess {

		/**
		 * Type for the on-exit handler.
		 */
		using OnExitHandler = std::function<void()>;

		virtual ~ChildProcess() = default;

		/**
		 * Forcibly kill this process.
		 */
		virtual void Kill() = 0;

		virtual int ExitCode() const = 0;

		// TODO: virtual int Pid() = 0;


		/**
		 * Assign an exit handler to this process.
		 *
		 * \param[in] handler Exit handler function.
		 */
		virtual void AssignExitHandler(OnExitHandler&& handler) = 0;

		virtual void Start(const std::string& commandline) = 0;

		virtual void Start(const std::vector<std::string>& command_args) = 0;
	};

	/**
	 * Create a child process.
	 */
	std::shared_ptr<ChildProcess> CreateChildProcess(boost::asio::io_context& ioc);


} // namespace collabvm::util

#endif // UTIL_CHILDPROCESS_H