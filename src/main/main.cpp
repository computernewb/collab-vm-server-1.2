#include <Arguments.h>
#include <iostream>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <boost/dll.hpp>
#include <plugin/PluginAPI.h>

// Test plugin hosting with Boost::DLL.
// This is only a test hosting thing, and won't be fully adopted (yet)
void test_plugin_host() {

	struct PluginApiImpl : public collabvm::plugin::IPluginApi {

		void WriteLogMessageImpl(const collabvm::plugin::utf8char* message) {
			std::cout << "Log message from plugin: " << reinterpret_cast<const char*>(message) << std::endl;
		}

		void* MallocImpl(std::size_t c) {
			return malloc(c);
		}

		void FreeImpl(void* p) {
			free(p);
		}

		PluginApiImpl() : IPluginApi() {
			COLLABVM_PLUGINABI_ASSIGN_VTFUNC(WriteLogMessage, &PluginApiImpl::WriteLogMessageImpl);
			COLLABVM_PLUGINABI_ASSIGN_VTFUNC(Malloc, &PluginApiImpl::MallocImpl);
			COLLABVM_PLUGINABI_ASSIGN_VTFUNC(Free, &PluginApiImpl::FreeImpl);
		}

	} pluginImpl;

	auto p = pluginImpl.Malloc(1);

	std::cout << "calling IPluginApi::Free\n";
	pluginImpl.Free(p);

	auto so = boost::dll::shared_library("plugins/helloworld.so");

	// verify ABI version.
	{
		auto collabvm_plugin_abi_version = so.get<int()>("collabvm_plugin_abi_version");

		// fuck
		if(!collabvm_plugin_abi_version)
			return;

		// big problem.
		if(collabvm_plugin_abi_version() != collabvm::plugin::PLUGIN_ABI_VERSION)
			return;
	}

	// now we can do the fun stuff!
	auto collabvm_plugin_init_api = so.get<void(collabvm::plugin::IPluginApi*)>("collabvm_plugin_init_api");

	// fuck
	if(!collabvm_plugin_init_api)
		return;

	// feed it plugin API
	collabvm_plugin_init_api(&pluginImpl);

	auto collabvm_plugin_make_serverplugin = so.get<collabvm::plugin::IServerPlugin*()>("collabvm_plugin_make_serverplugin");
	auto collabvm_plugin_delete_serverplugin = so.get<void(collabvm::plugin::IServerPlugin*)>("collabvm_plugin_delete_serverplugin");

	if(!collabvm_plugin_make_serverplugin || !collabvm_plugin_delete_serverplugin)
		return;

	auto plugin = collabvm_plugin_make_serverplugin();

	if(!plugin)
		return; // we have other issues

	// Initalize the plugin
	plugin->Init();

	// We're done with the plugin, tell the plugin to delete itself.
	collabvm_plugin_delete_serverplugin(plugin);
}

//#include <core/Server.h>

// Uncomment this to allow Asio to use multi-threading. WARNING: Might be buggy
//#define ENABLE_ASIO_MULTITHREADING

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

	// Test plugin hosting
	test_plugin_host();

	// Run the io_service on the main thread.
	ioc.run();

#ifdef ENABLE_ASIO_MULTITHREADING
	// Join ASIO completion handler threads when the server is stopping.
	for(auto& thread : threads)
		thread.join();
#endif

	return 0;
}
