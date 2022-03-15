//
// CollabVM Server
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#ifndef PIXELBOARD_MESSAGE_H
#define PIXELBOARD_MESSAGE_H

#include <cstdint>
#include <vector>
#include <memory>
#include <span>
#include <string_view>

namespace collabvm::websocket {

	/**
	 * Neat wrapper class for WebSocket messages.
	 */
	struct Message {
		enum class Type {
			Text,
			Binary
		};

		// trivial-ctor
		Message() = default;

		/**
		 * Constructor for creating a text WebSocket message.
		 *
		 * \param[in] sv string_view to pass in
		 */
		explicit Message(std::string_view sv);

		/**
		 * Constructor for creating a binary WebSocket message.
		 *
		 * \param sp Span to pass in
		 */
		explicit Message(std::span<std::uint8_t> sp);

		Message& operator=(const Message& other);

		/**
		 * Get the type of this message.
		 */
		[[nodiscard]] Type GetType() const;

		/**
		 * Gets the content of this Message as a string,
		 * if applicable (type == Type::String).
		 *
		 * Asserts the above condition.
		 */
		[[nodiscard]] std::string_view GetString() const;

		/**
		 * Get the content of this Message as a span<const uint8_t>,
		 * if applicable (type == Type::Binary).
		 *
		 * Asserts the above condition.
		 */
		[[nodiscard]] std::span<const std::uint8_t> GetBinary() const;

	   private:
		friend struct Client; // Client can access internal implementation details

		Type type {};
		std::vector<std::uint8_t> data_buffer;
	};

} // namespace collabvm::websocket

#endif // PIXELBOARD_MESSAGE_H
