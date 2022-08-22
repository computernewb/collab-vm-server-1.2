//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#include "WebSocketMessage.h"

#include <boost/assert.hpp>
#include <cstring>

namespace collab3::core::http {

	// FIXME: I might be able to use the compiler-default
	//  assignment? Dunno.
	WebSocketMessage& WebSocketMessage::operator=(const WebSocketMessage& other) {
		if(&other == this)
			// Make this a no-op for self-assignment.
			return *this;

		this->data_buffer = other.data_buffer;
		this->type = other.type;

		return *this;
	}

	WebSocketMessage::WebSocketMessage(std::string_view sv) :
		type(Type::Text) {
		data_buffer.resize(sv.size());
		std::memcpy(&data_buffer[0], sv.data(), sv.size());
	}

	WebSocketMessage::WebSocketMessage(std::span<std::uint8_t> sp) :
		type(Type::Binary) {
		data_buffer.resize(sp.size());
		std::memcpy(&data_buffer[0], sp.data(), sp.size());
	}

	WebSocketMessage::Type WebSocketMessage::GetType() const {
		return type;
	}

	std::string_view WebSocketMessage::GetString() const {
		BOOST_ASSERT_MSG(type == Type::Text, "this is not a Text WebSocket message");
		return { reinterpret_cast<const char*>(data_buffer.data()), data_buffer.size() };
	}

	std::span<const std::uint8_t> WebSocketMessage::GetBinary() const {
		BOOST_ASSERT_MSG(type == Type::Binary, "this is not a Binary WebSocket message");
		return std::span<const std::uint8_t> { data_buffer.data(), data_buffer.size() };
	}

	std::size_t WebSocketMessage::GetSize() const {
		return data_buffer.size();
	}

} // namespace collab3::core::http