//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

// Some fundamental types.

#pragma once

#include <cstdint>

namespace collab3::core {

	namespace detail {


		template<class T>
		struct Point {
			T x;
			T y;
		};

		// TODO: x-y-w-h. We're not windows.
		template<class T>
		struct Rect {
			T left;
			T top;
			T right;
			T bottom;

			/**
			 * Get the origin coordinate as a point.
			 * \return a Point<T> with the origin.
			 */
			constexpr Point<T> GetOrigin() const {
				return Point<T> { .x = left, .y = top };
			}

			/**
			 * Get the size of this rect.
			 * \return a Point<T> which contains the calculated size of the rect
			 */
			constexpr Point<T> GetSize() const {
				return Point<T> { .x = right - left, .y = bottom - top };
			}

			// more methods...
		};

	} // namespace detail


	/**
	 * Type used to communicate byte sizes.
	 * May become a literal class type later.
	 */
	using ByteSize = std::size_t;

	constexpr static inline ByteSize Kb = 1024; // bytes
	constexpr static inline ByteSize KB = 1000; // bytes

	constexpr static inline ByteSize Mb = 1024 * Kb;
	constexpr static inline ByteSize MB = 1000 * KB;

	constexpr static inline ByteSize Gb = 1024 * Mb;
	constexpr static inline ByteSize GB = 1000 * MB;

	/**
	 * A rectangle using uint32_t as its backing type.
	 */
	using Rect = detail::Rect<std::uint32_t>;

	/**
	 * A rectangle using uint16_t as its backing type.
	 */
	using Rect16 = detail::Rect<std::uint16_t>;

	/**
	 * A point using uint32_t as its backing type.
	 */
	using Point = detail::Point<std::uint32_t>;

	/**
	 * A point using uint16_t as its backing type.
	 */
	using Point16 = detail::Point<std::uint16_t>;

} // namespace collab3::core
