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

#include <core/Result.hpp>
#include <core/Error.hpp>

namespace collab3::core {


	enum class ConfigStoreErrorCode : std::uint8_t {
		Ok,
		InvalidType,
		ValueNonExistent
	};

	/**
	 * Specialization of ErrorCategory for ConfigStore::ErrorCode.
	 */
	template<>
	struct ::collab3::core::ErrorCategory<ConfigStoreErrorCode> {
		using CodeType = ConfigStoreErrorCode;

		static constexpr const char* Message(CodeType errorCode) {
			using enum ConfigStoreErrorCode;

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




		template<class T>
		using Result = Result<T, Error<ConfigStoreErrorCode>>;

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
				auto res = Is<T>();

				if(res.has_value()) {
					if(!res.value())
						return Result<T> { tl::unexpect, ConfigStoreErrorCode::InvalidType };
				} else if (!res) {
					return Result<T> { tl::unexpect, res.error() };
				}

				auto val = MaybeFetchValue();

				if(val.has_value())
					return std::get<T>(val.value());
				else
					return Result<T>{tl::unexpect, val.error()};
			}

			/**
			 * Get if this value is of type T.
			 *
			 * \return True if this value is of type T; false otherwise.
			 */
			template<class T>
			[[nodiscard]] inline Result<bool> Is() const {
				auto value = MaybeFetchValue();
				if(!value)
					return value.error();

				return std::holds_alternative<T>(value.value());
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

