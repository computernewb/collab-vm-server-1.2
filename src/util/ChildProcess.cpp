#include <util/ChildProcess.h>
#include <util/CommandLine.h>

#include <cassert>

#ifdef _WIN32
	// MinGW-w64 headers don't define this,
	// and Boost.Winapi seems to use this symbol in certain places.
	//
	// Oh well - still ~700 lines less platform dependent code.
	#ifndef __kernel_entry
		#define __kernel_entry
	#endif
#endif


#include <boost/process/child.hpp>

#include <boost/process/args.hpp>
#include <boost/process/async.hpp>
#include <boost/process/search_path.hpp>

namespace process = boost::process;



namespace collabvm::util {

	namespace {

		struct ChildProcessImpl : public std::enable_shared_from_this<ChildProcessImpl>, public ChildProcess {

			explicit ChildProcessImpl(boost::asio::io_context& ioc) 
				: ioc_(ioc) {
			}

			void Kill() override {
				// TODO: We should probably have a flag in here
				// which gets passed to the exit handler.
				//
				// If it's true, then the process was killed with Kill()
				// and not by some other force.
				
				if(process_.running())
					process_.terminate();
			}

			int ExitCode() const override {
				// TODO: assert(!process_.running())
				return exit_code_;
			}

			void AssignExitHandler(OnExitHandler&& handler) override {
				exit_handler_ = std::move(handler);
			}

			void Start(const std::string& commandline) override {
				auto split = SplitCommandLine(commandline);

				if(!split.has_value()) {
					assert(false && "However you managed this, I'm impressed.");
					return;
				}

				Start(split.value());
			}


			void Start(const std::vector<std::string>& command_args) override {
				using namespace std::placeholders;

				auto exit_handler = std::bind(&ChildProcessImpl::ExitHandlerInternal, shared_from_this(), _1, _2);

				if(command_args.size() < 2) {	
					process_ = process::child(process::search_path(command_args[0]), process::on_exit = exit_handler, ioc_);
				} else {
					auto argsified = std::vector<std::string>{command_args.begin() + 1, command_args.end()};
					process_ = process::child(process::search_path(command_args[0]), process::args = argsified, process::on_exit = exit_handler, ioc_);
				}
			}

		private:

			void ExitHandlerInternal(int exitCode, const std::error_code& ec) {
				if(ec)
					return;

				exit_code_ = exitCode;

				if(exit_handler_)
					exit_handler_();
			}

			boost::asio::io_context& ioc_;

			process::child process_;
			OnExitHandler exit_handler_{};
			int exit_code_{};
		};

	}

	std::shared_ptr<ChildProcess> CreateChildProcess(boost::asio::io_context& ioc) {
		return std::make_shared<ChildProcessImpl>(ioc);
	}
	

} // namespace collabvm::util