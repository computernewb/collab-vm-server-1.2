#include <Arguments.h>
#include <iostream>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <websocket/Server.h>

// Uncomment this to allow Asio to use multi-threading. WARNING: Might be buggy
//#define ENABLE_ASIO_MULTITHREADING

#if 0
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
#endif

int main(int argc, char** argv) {
	// maybe global instance this so things can access it?
	collabvm::main::Arguments args;
	boost::asio::io_context ioc;

	args.Process(argc, argv);

	// Set up Ctrl+C handler
	boost::asio::signal_set interruptSignal(ioc, SIGINT, SIGTERM);
	interruptSignal.async_wait([&](const boost::system::error_code& ec, int sig) {
		std::cout << "Stopping...\n";
		ioc.stop();
	});

	auto server = std::make_shared<collabvm::websocket::Server>(ioc);

	// Test echo + user-data server.

	struct MyData {
		int n {};
	};

	server->SetOpen([](std::weak_ptr<collabvm::websocket::Client> client) {
		if(auto sp = client.lock()) {
			// Assign the given user data type to this connection.
			// We can now use GetUserData<MyData> to get the instance of MyData for each connection.
			sp->SetUserData<MyData>();
			std::cout << "Connection opened from " << sp->GetRemoteAddress().to_string() << '\n';
		}
	});

	server->SetMessage([](std::weak_ptr<collabvm::websocket::Client> client, std::shared_ptr<const collabvm::websocket::Message> message) {
		if(auto sp = client.lock()) {
			if(message->GetType() == collabvm::websocket::Message::Type::Text) {
				std::cout << "Message from " << sp->GetRemoteAddress().to_string() << ": " << message->GetString() << '\n';
				sp->GetUserData<MyData>().n++;
				sp->Send(message);
			}
		}
	});

	server->SetClose([](std::weak_ptr<collabvm::websocket::Client> client) {
		if(auto sp = client.lock()) {
			std::cout << "Connection closed from " << sp->GetRemoteAddress().to_string() << '\n';
			std::cout << "MyData::n for this connection: " << sp->GetUserData<MyData>().n << '\n';
		}
	});

	server->Run(args.GetListenAddress(), args.GetPort());

	// FIXME: If this is re-enabled, maybe default to (nproc() / 2) -1,
	// 	but add a --threads command-line argument?
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
