//
// Created by lily on 8/22/22.
//

#ifndef CORE_CONFIG_CONFIGSTORE_H
#define CORE_CONFIG_CONFIGSTORE_H

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

namespace collab3::core {

	/**
	 * Configuration store.
	 *
	 * This serves as a basis for backends to store
	 * configuration data as they parse it.
	 */
	struct ConfigStore {
		using ConfigKey = std::string;

		// clang-format off

		using ConfigValue = std::variant<
			bool,

			// Integer types (I know this is wasteful but /shrug)
			std::uint64_t,
			std::int64_t,

			std::string

			// Array? I don't know what the purpose of that would be.
		>;
		// clang-format on

	   private:
		struct ArrayProxy;
	   public:

		bool ValueExists(const ConfigKey& key) const;

		ConfigStore::ArrayProxy operator[](const ConfigKey& key);

		template<class T>
		void AddValue(const ConfigKey& key, const T& value) {
			if(!ValueExists(key))
				valueMap.insert({ key, value });
		}

	   private:
		/**
		 * Array proxy object.
		 */
		struct ArrayProxy {
			struct InvalidType : public std::exception {
				[[nodiscard]] const char* what() const noexcept override {
					return "As<T>() type different than stored type. Use Is<T>().";
				}
			};

			/**
			 * Get this object as type T.
			 *
			 * Note that this function throws if you attempt
			 * to get a value as a invalid type.
			 *
			 * \returns The value object.
			 */
			template<class T>
			[[nodiscard]] T& As() const {
				if(!Is<T>())
					throw InvalidType();

				return std::get<T>(underlyingValue);
			}

			/**
			 * Get if this value is of type T.
			 *
			 * \return True if this value is of type T; false otherwise.
			 */
			template<class T>
			[[nodiscard]] constexpr bool Is() const {
				return std::holds_alternative<T>(underlyingValue);
			}

		   private:
			friend ConfigStore;

			constexpr explicit ArrayProxy(ConfigValue& value) :
				underlyingValue(value) {
			}

			ConfigValue& underlyingValue;
		};

		std::unordered_map<ConfigKey, ConfigValue> valueMap;
	};

} // namespace collab3::core

#endif //CORE_CONFIG_CONFIGSTORE_H
