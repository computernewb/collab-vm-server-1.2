//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#ifndef PROTO_HTTP_CLIENT_H
#define PROTO_HTTP_CLIENT_H

#include <any>
#include <atomic>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <optional>

#include "ForwardDeclarations.h"
#include "NetworkingTSCompatibility.h"
#include "WebSocketMessage.h"

namespace collab3::proto::http {

	/**
	 * A WebSocket client.
	 */
	struct WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
		enum class State {
			Closed,
			Validating,
			Open,
			Closing
		};

		WebSocketClient(tcp::socket&& socket, std::shared_ptr<Server> server);

		~WebSocketClient();

		template<class T>
		T& GetUserData() noexcept {
			auto* p = std::any_cast<T>(&userData);
			assert(p && "user type T isn't actually stored here!");
			return *p;
		}

		template<class T>
		const T& GetUserData() const noexcept {
			auto* p = std::any_cast<T>(&userData);
			assert(p && "user type T isn't actually stored here!");
			return *p;
		}

		/**
		 * Set the user data to this type.
		 *
		 * \tparam T Type. Must be default-constructable. Otherwise I won't like you anymore.
		 */
		template<class T>
		void SetUserData() {
			userData = std::make_any<T>();
		}

		net::ip::address GetRemoteAddress();

		/**
		 * Get the current state of this WebSocket client.
		 */
		State GetState() const;

		// Functions only valid during the Verify state
		void SelectSubprotocol(const std::string& subprotocol);

		// Only valid when Open

		std::string_view GetSelectedSubprotocol();

		/**
		 * Send a message.
		 */
		void Send(std::shared_ptr<const WebSocketMessage> message);

		/**
		 * Close the WebSocket connection.
		 */
		void Close();

		// If you call this I won't like you anymore

		void Run(bhttp::request<bhttp::string_body>&& request);

	   private:
		void OnAccept(const boost::system::error_code& ec);

		void ReadMessage();

		void OnRead(const boost::system::error_code& ec, std::size_t bytes_transferred);

		/**
		 * Write a single WebSocketMessage.
		 *
		 * \param[in] message The message to write
		 */
		void WriteMessage(std::shared_ptr<const WebSocketMessage> message);

		void OnWrite(const boost::system::error_code& ec, std::size_t bytes_transferred);

		/**
		 * Perform a hard close of the TCP socket.
		 * This is done when validation fails.
		 */
		void HardClose();

		/**
		 * user data.
		 */
		std::any userData;

		beast::websocket::stream<beast::tcp_stream> wsStream;
		net::ip::address cachedAddress;

		beast::flat_buffer messageBuffer;

		std::atomic<State> state { State::Validating };

		// basically just random data

		bhttp::request<bhttp::string_body> upgradeRequest;
		std::optional<std::string> selectedSubprotocol;

		// fwd decl - don't want it here!
		struct MessageQueue;

		/**
		 * WebSocketMessage queue instance.
		 */
		std::unique_ptr<MessageQueue> messageQueue;

		std::shared_ptr<Server> server;
	};

} // namespace collab3::proto::http

#endif // PIXELBOARD_CLIENT_H
