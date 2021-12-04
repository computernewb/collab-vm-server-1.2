//
// Created by lily on 11/24/2021.
//

#include <websocket/Message.h>

#include <cstring>
#include <boost/assert.hpp>

namespace collabvm::websocket {

	// FIXME: I might be able to use the compiler-default
	//  assignment? Dunno.
	Message& Message::operator=(const Message& other) {
		if(&other == this)
			// Make this a no-op for self-assignment.
			return *this;

		this->data_buffer = other.data_buffer;
		this->type = other.type;

		return *this;
	}

	Message::Message(std::string_view sv)
		: type(Type::Text) {
		data_buffer.resize(sv.size());
		std::memcpy(&data_buffer[0], sv.data(), sv.size());
	}

	Message::Message(std::span<std::uint8_t> sp)
		: type(Type::Binary) {
		data_buffer.resize(sp.size());
		std::memcpy(&data_buffer[0], sp.data(), sp.size());
	}

	Message::Type Message::GetType() const {
		return type;
	}

	std::string_view Message::GetString() const {
		BOOST_ASSERT_MSG(type == Type::Text, "this is not a Text websocket message");
		return {
			reinterpret_cast<const char*>(data_buffer.data()),
			data_buffer.size()
		};
	}

	std::span<const std::uint8_t> Message::GetBinary() const {
		BOOST_ASSERT_MSG(type == Type::Binary, "this is not a Binary websocket message");
		return std::span<const std::uint8_t> { data_buffer.data(), data_buffer.size() };
	}

} // namespace collabvm::websocket