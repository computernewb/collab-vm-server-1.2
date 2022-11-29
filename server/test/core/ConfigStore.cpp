//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

#include <catch2/catch_test_macros.hpp>
#include "../Collab3TestExtensions.hpp"

#include <core/config/ConfigStore.hpp>


using namespace collab3::core;

TEST_CASE("ConfigStore default construction", "[ConfigStore]") {
	ConfigStore store;

	// This shouldn't cause any problems.
	REQUIRE_NOTHROW(store["abc"].Exists());
}

TEST_CASE("ConfigStore value manipulation", "[ConfigStore]") {
	GIVEN("a ConfigStore") {
		ConfigStore store;

		THEN("Looking up a non-existent value fails regardless of type") {
			REQUIRE_ERROR_AS(store["XXXINVALID_KEY"].As<bool>(), ConfigStoreErrorCode::ValueNonExistent);
			REQUIRE_ERROR_AS(store["XXXINVALID_KEY"].As<std::string>(), ConfigStoreErrorCode::ValueNonExistent);
			REQUIRE_ERROR_AS(store["XXXINVALID_KEY"].As<std::uint64_t>(), ConfigStoreErrorCode::ValueNonExistent);
			REQUIRE_ERROR_AS(store["XXXINVALID_KEY"].As<std::int64_t>(), ConfigStoreErrorCode::ValueNonExistent);
		}

		THEN("Inserting a value of a given type") {
			store["value"].Set(static_cast<std::uint64_t>(32));

			AND_THEN("works as expected, creating the value") {
				REQUIRE(store["value"].Exists());
				REQUIRE(store["value"].Is<std::uint64_t>());
			}

			AND_THEN("Conversion to the right type succeeds") {
				REQUIRE_NOTHROW(store["value"].As<std::uint64_t>());
			}

			AND_THEN("Conversion to the wrong type doesn't succeed") {

				REQUIRE_ERROR_AS(store["value"].As<std::string>(), ConfigStoreErrorCode::InvalidType);
			}

			AND_THEN("Removing works") {
				store["value"].Remove();

				// Let's make sure the value really was removed
				REQUIRE(store["value"].Exists() == false);

				REQUIRE_ERROR_AS(store["value"].As<std::uint64_t>(), ConfigStoreErrorCode::ValueNonExistent);
			}
		}
	}
}
