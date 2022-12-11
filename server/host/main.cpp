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

	// Set some values
	store["test"].Set("Does this work?");
	store["test2"].Set(true);
	store["test3"].Set(static_cast<std::int64_t>(120));
	store["test4"].Set(static_cast<std::uint64_t>(120));

	// Try reading them out
	std::cout << "test: \"" << store["test"].As<std::string>().value() << "\"\n";
	std::cout << "test2: \"" << store["test2"].As<bool>().value() << "\"\n";
	std::cout << "test3: \"" << store["test3"].As<std::int64_t>().value() << "\"\n";
	std::cout << "test4: \"" << store["test4"].As<std::uint64_t>().value() << "\"\n";
	return 0;
}
