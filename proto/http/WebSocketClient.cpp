//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#include "WebSocketClient.h"

#include <iostream>

#include "HttpUtils.h"
#include "Server.h"

namespace collab3::proto::http {

	struct WebSocketClient::MessageQueue {
		/**
		 * The max amount of WebSocket messages we buffer
		 * inside of this send queue.
		 */
		constexpr static auto MAX_MESSAGES = 512;

		WebSocketClient& self;
		std::vector<std::shared_ptr<const WebSocketMessage>> messages;

		explicit MessageQueue(WebSocketClient& self) :
			self(self) {
			messages.reserve(MAX_MESSAGES);
		}

		[[nodiscard]] bool IsFull() const {
			return messages.size() >= MAX_MESSAGES;
		}

		std::shared_ptr<const WebSocketMessage>& GetFront() {
			return messages.front();
		}

		void OnWrite() {
			if(self.state.load() == State::Closing)
				return;

			messages.erase(messages.begin());

			// If there are more messages,
			// keep writing them
			if(!messages.empty())
				self.WriteMessage(messages.front());
		}

		void Push(std::shared_ptr<const WebSocketMessage> message) {
			if(IsFull())
				return;

			if(self.state.load() == State::Closing)
				return;

			messages.push_back(message);

			// Begin writing if we can
			if(messages.size() == 1)
				self.WriteMessage(messages.front());
		}
	};

	WebSocketClient::WebSocketClient(tcp::socket&& socket, std::shared_ptr<Server> server) :
		server(server),
		wsStream(std::move(socket)) {
		messageQueue = std::make_unique<MessageQueue>(*this);
	}

	// Generate the trivial-dtor here
	// where WebSocketClient::MessageQueue is fully defined.
	WebSocketClient::~WebSocketClient() = default;

	void WebSocketClient::SelectSubprotocol(const std::string& subprotocol) {
		selectedSubprotocol = subprotocol;
	}

	std::string_view WebSocketClient::GetSelectedSubprotocol() {
		assert(selectedSubprotocol.has_value());
		return selectedSubprotocol.value();
	}

	net::ip::address WebSocketClient::GetRemoteAddress() {
		return cachedAddress;
	}

	WebSocketClient::State WebSocketClient::GetState() const {
		return state.load();
	}

	void WebSocketClient::Run(bhttp::request<bhttp::string_body>&& request) {
		// plop in the upgrade request
		upgradeRequest = std::move(request);

		net::post(wsStream.get_executor(), [self = shared_from_this()] {
			self->cachedAddress = self->wsStream.next_layer().socket().remote_endpoint().address();

			// TODO: proxy load balancer support.
			// Unlike CVM1 i want it to be a runtime option.

			self->wsStream.set_option(beast::websocket::stream_base::decorator([self](beast::websocket::response_type& res) {
				SetCommonResponseFields(res);

				// If a subprotocol has been selected, add the Sec-Websocket-Protocol header to reflect that.
				// Firefox wrongly allows connections if we respond with an invalid handshake without the subprotocol,
				// while Chromium does the correct thing and doesn't continue the handshake.

				if(self->selectedSubprotocol.has_value())
					res.set(bhttp::field::sec_websocket_protocol, self->selectedSubprotocol.value());
			}));

			// TODO: permessage-deflate?

			if(self->server->validate_handler) {
				// If we *have* an validate handler,
				// call it.
				if(!self->server->validate_handler(self->weak_from_this()))
					// Validate handler failed, close without
					// calling the close handler.
					return self->HardClose();
			}

			// We can finally accept the connection.
			self->wsStream.async_accept(self->upgradeRequest, beast::bind_front_handler(&WebSocketClient::OnAccept, self->shared_from_this()));
		});
	}

	void WebSocketClient::OnAccept(const boost::system::error_code& ec) {
		if(ec)
			return;

		// Well, we got here, so we're open now!
		state.store(State::Open);

		// Call the defined Open handler.
		if(server->open_handler)
			server->open_handler(weak_from_this());

		// Start the messsage reading loop
		ReadMessage();
	}

	void WebSocketClient::ReadMessage() {
		wsStream.async_read(messageBuffer, beast::bind_front_handler(&WebSocketClient::OnRead, shared_from_this()));
	}

	void WebSocketClient::OnRead(const boost::system::error_code& ec, std::size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);

		if(ec == beast::websocket::error::closed)
			return Close();

		// this is a little grody :(

		if(wsStream.got_binary()) {
			if(server->message_handler)
				server->message_handler(weak_from_this(),
										std::make_shared<WebSocketMessage>(std::span<std::uint8_t> {
										reinterpret_cast<std::uint8_t*>(messageBuffer.data().data()),
										messageBuffer.size() }));
		} else if(wsStream.got_text()) {
			if(server->message_handler)
				server->message_handler(
				weak_from_this(),
				std::make_shared<WebSocketMessage>(
				std::string_view { reinterpret_cast<char*>(messageBuffer.data().data()), messageBuffer.size() }));
		}

		// The WebSocketMessage constructor internally copies data, so we can clear the buffer once we have called the message handler and it's returned.
		messageBuffer.clear();

		// Read another WebSocket message
		ReadMessage();
	}

	void WebSocketClient::WriteMessage(std::shared_ptr<const WebSocketMessage> message) {
		if(!message)
			return;

		if(message->GetType() == WebSocketMessage::Type::Text)
			wsStream.text(true);
		else if(message->GetType() == WebSocketMessage::Type::Binary)
			wsStream.binary(true);

		wsStream.async_write(net::buffer(message->data_buffer),
							 beast::bind_front_handler(&WebSocketClient::OnWrite, shared_from_this()));
	}

	void WebSocketClient::OnWrite(const boost::system::error_code& ec, std::size_t bytes_transferred) {
		if(ec)
			return Close();

		// notify the message queue that we've finished writing a message
		messageQueue->OnWrite();
	}

	void WebSocketClient::Close() {
		state.store(State::Closing);
		wsStream.async_close(beast::websocket::close_reason(beast::websocket::close_code::normal),
							 [self = shared_from_this()](const boost::system::error_code& ec) {
								 self->state.store(State::Closed);

								 if(self->server->close_handler)
									 self->server->close_handler(self->weak_from_this());

								 // After this all strong client handles should be relinquished
								 // so this object dies
							 });
	}

	void WebSocketClient::Send(std::shared_ptr<const WebSocketMessage> message) {
		if(!messageQueue->IsFull())
			messageQueue->Push(message);
	}

	void WebSocketClient::HardClose() {
		boost::system::error_code ec;
		state.store(State::Closing);
		wsStream.next_layer().socket().close(ec);
	}

} // namespace collab3::proto::http