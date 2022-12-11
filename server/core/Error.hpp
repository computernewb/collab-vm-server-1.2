//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

#pragma once

namespace collab3::core {

	// this isn't great, so I might replace it? idk

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

		//static constexpr const char* Message(EC);
		//static EC OkSymbol();
		//static bool Ok(EC);
	};

	/**
	 * An error. Light-weight wrapper over an error code.
	 *
	 * Error category storage is handled as a part of the type, so all Error(s)
	 * are literal and trivial, and only sizeof(EC).
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
		constexpr Error() :
			Error(Category::OkSymbol()) {
		}

		constexpr Error(typename Category::CodeType code) :
			ec(code) {
			// not a great spot for this kind of thing, but oh well
			static_assert(sizeof(Error) == sizeof(EC), "Basic assumptions with your compiler seem to be horribly wrong");
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
		typename Category::CodeType ec;
	};

	template<class EC>
	Error(EC code) -> Error<EC>;

} // namespace collab3::core