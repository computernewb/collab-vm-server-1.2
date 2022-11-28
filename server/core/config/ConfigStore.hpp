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

namespace collab3::core {

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

			// Array? I don't know what the purpose of that would be.
		>;
		// clang-format on

		enum class ErrorCode : std::uint8_t {
			Ok,
			InvalidType,
			ValueNonExistent
		};

		// TODO: Remove exception, we will instead return a Result<T>

		struct InvalidType : public std::exception {
			[[nodiscard]] const char* what() const noexcept override {
				return "As<T>() type different than stored type. Use Is<T>() to type-check.";
			}
		};

		struct NonExistentValue : public std::exception {
			[[nodiscard]] const char* what() const noexcept override {
				return "Non-existent value lookup. Use Set() beforehand or Exists() to check.";
			}
		};

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
			[[nodiscard]] const T& As() const {
				if(!Is<T>())
					throw InvalidType();

				return std::get<T>(MaybeFetchValue());
			}

			/**
			 * Get if this value is of type T.
			 *
			 * \return True if this value is of type T; false otherwise.
			 */
			template<class T>
			[[nodiscard]] constexpr bool Is() const {
				return std::holds_alternative<T>(MaybeFetchValue());
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
			[[nodiscard]] ConfigValue& MaybeFetchValue() const;

			ConfigStore& underlyingStore;
			const ConfigKey& key;
		};

		ConfigStore::ArrayProxy operator[](const ConfigKey& key);

		// More functions?
		// (iteration for saving, too, probably.)

	   private:
		std::unordered_map<ConfigKey, ConfigValue> valueMap;
	};

	/**
	 * Specialization of ErrorCategory for ConfigStore::ErrorCode.
	 */
	template<>
	struct ErrorCategory<ConfigStore::ErrorCode> {
		using CodeType = ConfigStore::ErrorCode;

		static constexpr const char* Message(CodeType errorCode) {
			using enum ConfigStore::ErrorCode;

			switch(errorCode) {
				case Ok: return "No error.";
				case InvalidType: return "As<T>() type different than stored type.";
				case ValueNonExistent: return "Non-existent value lookup.";

				default: return "";
			}
		}

		static constexpr CodeType OkSymbol() {
			return CodeType::Ok;
		}

		static constexpr bool Ok(CodeType error) {
			return error == OkSymbol();
		}
	};

} // namespace collab3::core

