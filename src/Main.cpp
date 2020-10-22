#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <map>
#include <thread>
#include "CollabVM.h"
#ifndef _WIN32
#include "StackTrace.hpp"
#define STACKTRACE PrintStackTrace();
#ifdef __CYGWIN__
int backtrace (void **buffer, int size) { return 0;}
char ** backtrace_symbols (void *const *buffer, int size) { return 0;}
void backtrace_symbols_fd (void *const *buffer, int size, int fd) {}
#endif
#else
#include <windows.h>
#endif

std::shared_ptr<CollabVMServer> server_;
boost::asio::io_service service_;
std::unique_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work(service_));

bool stopping = false;

void SignalHandler()
{
	if (stopping)
		return;
	stopping = true;

	try
	{
		try
		{
			std::cout << "\nShutting down..." << std::endl;
			work.reset();
			server_->Stop();
		}
		catch (const websocketpp::exception& ex)
		{
			std::cout << "A websocketpp exception was thrown: " << ex.what() << std::endl;
			throw;
		}
		catch (const std::exception& ex)
		{
			std::cout << "An exception was thrown: " << ex.what() << std::endl;
			throw;
		}
		catch (...)
		{
			std::cout << "An unknown exception occurred\n";
			throw;
		}
	}
	catch (...)
	{
		server_->Stop();
#ifndef _WIN32
		PrintStackTrace();
#endif
	}
}

#ifdef _WIN32
BOOL CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
		SignalHandler();
		return TRUE;
	}
	return FALSE;
}
#endif

static void SegFaultHandler(int signum)
{
	std::cout << "A segmentation fault occured\n";
#ifndef _WIN32
	PrintStackTrace();
#endif
	exit(-1);
}

template<typename F>
void WorkerThread(F func)
{
	try
	{
		try
		{
			func();
		}
		catch (const websocketpp::exception& ex)
		{
			std::cout << "A websocketpp exception was thrown: " << ex.what() << std::endl;
			throw;
		}
		catch (const std::exception& ex)
		{
			std::cout << "An exception was thrown: " << ex.what() << std::endl;
			throw;
		}
		catch (...)
		{
			// TODO std::current_exception()
			std::cout << "An unknown exception occurred\n";
			throw;
		}
	}
	catch (...)
	{
#ifndef _WIN32
		PrintStackTrace();
#endif
	}

	server_->Stop();
	work.reset();
}


void IgnorePipe() {
	// Ignore SIGPIPE to prevent LibVNCClient from crashing
#ifndef _WIN32
	struct sigaction pipe;
	pipe.sa_handler = SIG_IGN;
	pipe.sa_flags = 0;
	if(sigaction(SIGPIPE, &pipe, 0) == -1) {
		std::cout << "Failed to ignore SIGPIPE. Crashes *may* occur if someone somehow makes input occur during a reset\n";
	}
#endif
}

#ifndef UNIT_TEST
int main(int argc, char* argv[])
{
	try
	{
		if (argc < 2 || argc > 3)
		{
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
		if (i != s.length() || !(port > 0 && port <= UINT16_MAX))
		{
			std::cout << "Invalid port for WebSocket server." << std::endl;
			return -1;
		}

		std::cout << "Collab VM Server started" << std::endl;

		// Set up Ctrl+C handler
#ifdef _WIN32
		if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
			std::cout << "Error setting console control handler." <<
			std::endl << "Ctrl+C will not exit cleanly." << std::endl;
#else
		struct sigaction sa;
		sa.sa_handler = &SegFaultHandler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;
		if (sigaction(SIGSEGV, &sa, NULL) == -1)
			std::cout << "Failed to set segmentation violation signal handler" << std::endl;

		boost::asio::signal_set interruptSignal(service_, SIGINT, SIGTERM);
		interruptSignal.async_wait(std::bind(&SignalHandler));
		IgnorePipe();
#endif

		server_ = std::make_shared<CollabVMServer>(service_);

		std::thread t([] { WorkerThread([] { service_.run(); }); });

		WorkerThread([&] { server_->Run(port, argc > 2 ? argv[2] : "http"); });

		// TODO: Replace this with t.detach()
		// and have a little busy loop waiting for the processing thread to stop
		// once it has, the loop will break
		t.join();
	}
	catch (websocketpp::exception const & e)
	{
		std::cout << "An exception was thrown:" << std::endl;
		std::cout << e.what() << std::endl;
#ifndef _WIN32
		PrintStackTrace();
#endif
#ifdef _DEBUG
		throw;
#endif
		return -1;
	}
	catch (...)
	{
#ifdef _DEBUG
		throw;
#endif
		return -1;
	}
	return 0;
}
#endif
