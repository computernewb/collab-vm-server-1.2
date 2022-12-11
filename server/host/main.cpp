//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

// Simple host

#include <iostream>
#include <core/config/ConfigStore.hpp>

int main(int argc, char** argv) {
	collab3::core::ConfigStore store;

	// Set some values in the store.
	store["test"].Set("Does this work?");
	store["test2"].Set(true);
	store["test3"].Set(static_cast<std::int64_t>(120));
	store["test4"].Set(static_cast<std::uint64_t>(120));

	// read them out, handling the error (if one exists)

	store["test"].As<std::string>()
		.map([](auto value) {
			std::cout << "test: \"" << value << "\"\n";
		})
		.map_error([](auto error) {
			std::cout << "err (test): \"" << error.Message() << "\"\n";
		});

	store["test2"].As<bool>()
		.map([](auto value) {
			std::cout << "test2: \"" << value << "\"\n";
		})
		.map_error([](auto error) {
			std::cout << "err (test2): \"" << error.Message() << "\"\n";
		});

	store["test3"].As<std::int64_t>()
		.map([](auto value) {
			std::cout << "test3: \"" << value << "\"\n";
		})
		.map_error([](auto error) {
			std::cout << "err (test3): \"" << error.Message() << "\"\n";
		});

	store["test4"].As<std::uint64_t>()
		.map([](auto value) {
			std::cout << "test4: \"" << value << "\"\n";
		})
		.map_error([](auto error) {
			std::cout << "err (test4): \"" << error.Message() << "\"\n";
		});
	return 0;
}
