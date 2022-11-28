#pragma once

namespace collab3::core {

	/**
	 * An error category. Provides functions for working
	 * with a single error code type.
	 *
	 * Needs to be specialized.
	 *
	 * \tparam EC
	 */
	template<class EC>
	struct ErrorCategory {
		using CodeType = EC;

		// This class should not be created.
		constexpr ErrorCategory() = delete;

		static constexpr const char* Message(EC);
		static EC OkSymbol();
		static bool Ok(EC);
	};

	/**
	 * An error. Light-weight wrapper over an ErrorCategory<EC> instance.
	 *
	 * Error category storage is handled as a part of the type, so all Error(s)
	 * are literal and trivial.
	 *
	 * This type *can* be directly used if desired, however it's recommended
	 * to use it as part of an Expected error argument.
	 *
	 * \tparam EC 		Error code type.
	 * \tparam Category The error category.
	 */
	template<class EC, class Category = ErrorCategory<EC>>
	struct Error {
		using CategoryType = Category;

		// Ok error (uses the OkSymbol)
		constexpr Error() = default;

		constexpr Error(typename Category::CodeType code)
			: ec(code) {

		}

		constexpr const char* Message() {
			return Category::Message(ec);
		}

		constexpr operator bool() const {
			return Category::Ok(ec);
		}

		constexpr operator typename Category::CodeType() const {
			return ec;
		}

	   private:
		typename Category::CodeType ec{Category::OkSymbol()};
	};

}