//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

// Some fundamental types.

#ifndef CORE_FUNDAMENTALTYPES_H
#define CORE_FUNDAMENTALTYPES_H

#include <stdint.h>

namespace collab3::core {

	namespace detail {

		template<class T>
		struct Point {
			T x;
			T y;
		};

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
				return Point<T> {
					.x = left,
					.y = top
				};
			}

			/**
			 * Get the size of this rect.
			 * \return a Point<T> which contains the calculated size of the rect
			 */
			constexpr Point<T> GetSize() const {
				return Point<T> {
					.x = right - left,
					.y = bottom - top
				};
			}

			// more methods...
		};

	} // namespace detail

	/**
	 * A rectangle using uint32_t as its backing type.
	 */
	using Rect = detail::Rect<uint32_t>;

	/**
	 * A rectangle using uint16_t as its backing type.
	 */
	using Rect16 = detail::Rect<uint16_t>;

	/**
	 * A point using uint32_t as its backing type.
	 */
	using Point = detail::Point<uint32_t>;

	/**
	 * A point using uint16_t as its backing type.
	 */
	using Point16 = detail::Point<uint16_t>;

} // namespace collab3::core

#endif //CORE_FUNDAMENTALTYPES_H
