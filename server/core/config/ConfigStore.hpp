//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include <core/Error.hpp>
#include <core/Result.hpp>

namespace collab3::core {

	enum class ConfigStoreErrorCode : std::uint8_t {
		Ok,
		InvalidType,
		ValueNonExistent
	};

	/**
	 * Specialization of ErrorCategory for ConfigStoreErrorCode.
	 */
	template<>
	struct ErrorCategory<ConfigStoreErrorCode> {
		using CodeType = ConfigStoreErrorCode;

		static constexpr const char* Message(CodeType errorCode) {
			using enum ConfigStoreErrorCode;

			// clang-format off
			switch(errorCode) {
				case Ok:				return "No error.";
				case InvalidType:		return "Requested type invalid for stored value.";
				case ValueNonExistent:	return "Requested value does not exist.";
				default:				return "";
			}
			// clang-format on
		}

		static constexpr CodeType OkSymbol() {
			return CodeType::Ok;
		}

		static constexpr bool Ok(CodeType error) {
			return error == OkSymbol();
		}
	};

	/**
	 * Configuration store.
	 *
	 * This serves as a basis for configuration backends to store
	 * configuration data, and for code to read from or set additional
	 * values as you go.
	 */
	struct ConfigStore {
		using ConfigKey = std::string;

		// clang-format off

		using ConfigValue = std::variant<
			bool,

			std::uint64_t,
			std::int64_t,

			std::string
		>;
		// clang-format on

		template<class T>
		using Result = core::Result<T, Error<ConfigStoreErrorCode>>;

		/**
		 * Array proxy object, used to allow a sane(r) API.
		 */
		struct ArrayProxy {
			/**
			 * Get this object as type T.
			 *
			 * Note that this function throws if you attempt
			 * to get a value as a invalid type.
			 *
			 * \returns The value object.
			 */
			template<class T>
			[[nodiscard]] Result<T> As() const {
				// clang-format off
				return MaybeFetchValue()
					.and_then([](auto variant) {
						if(std::holds_alternative<T>(variant))
							return Result<T> { std::get<T>(variant) };

						return Result<T> { tl::unexpect, ConfigStoreErrorCode::InvalidType };
					});

				// clang-format on
			}

			/**
			 * Get if this value is of type T.
			 *
			 * \return True if this value is of type T; false otherwise.
			 */
			template<class T>
			[[nodiscard]] inline Result<bool> Is() const {
				// clang-format off
				return MaybeFetchValue()
					.and_then([](auto variant) {
						return Result<bool>(std::holds_alternative<T>(variant));
					});
				// clang-format on
			}

			template<class T>
			constexpr void Set(const T& value) {
				SetBase(ConfigValue { value });
			}

			[[nodiscard]] bool Exists() const noexcept;

			/**
			 * Remove this value.
			 */
			void Remove();

		   private:
			friend ConfigStore;

			constexpr explicit ArrayProxy(ConfigStore& store, const ConfigKey& key) noexcept :
				underlyingStore(store),
				key(key) {
			}

			void SetBase(const ConfigValue& value);
			[[nodiscard]] Result<ConfigValue> MaybeFetchValue() const;

			ConfigStore& underlyingStore;
			const ConfigKey& key;
		};

		ConfigStore::ArrayProxy operator[](const ConfigKey& key);

		// More functions?
		// (iteration for saving, too, probably.)

	   private:
		std::unordered_map<ConfigKey, ConfigValue> valueMap;
	};

} // namespace collab3::core
