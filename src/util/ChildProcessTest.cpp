// A simple test of the new child process apis.

#include <cstdio>

#include <boost/asio.hpp>

#include <util/ChildProcess.h>


int main() {
	boost::asio::io_context service;

	auto qemu = collabvm::util::StartChildProcess(service, "qemu-system-x86_64 -accel kvm", [](int exitCode, const std::error_code& ec) {
		std::printf("child process exited with code %d\n", exitCode);
	});

	service.run();

	return 0;
}