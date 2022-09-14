#include <catch2/catch.hpp>
#include <core/config/ConfigStore.h>

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
			REQUIRE_THROWS_AS(store["XXXINVALID_KEY"].As<bool>(), ConfigStore::ArrayProxy::NonExistentValue);
			REQUIRE_THROWS_AS(store["XXXINVALID_KEY"].As<std::string>(), ConfigStore::ArrayProxy::NonExistentValue);
			REQUIRE_THROWS_AS(store["XXXINVALID_KEY"].As<std::uint64_t>(), ConfigStore::ArrayProxy::NonExistentValue);
			REQUIRE_THROWS_AS(store["XXXINVALID_KEY"].As<std::int64_t>(), ConfigStore::ArrayProxy::NonExistentValue);
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

			AND_THEN("Conversion to the right type succeeds") {
				REQUIRE_THROWS_AS(store["value"].As<std::string>(), ConfigStore::ArrayProxy::InvalidType);
			}

			AND_THEN("Removing works") {
				store["value"].Remove();

				// Make sure the value really was removed.
				REQUIRE(store["value"].Exists() == false);
				REQUIRE_THROWS_AS(store["value"].As<std::uint64_t>(), ConfigStore::ArrayProxy::NonExistentValue);
			}
		}
	}
}
