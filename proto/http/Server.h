//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#ifndef PROTO_HTTP_SERVER_H
#define PROTO_HTTP_SERVER_H

#include <boost/asio/io_context.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

// to pull in some stuff we don't wanna
#include "ForwardDeclarations.h"
#include "NetworkingTSCompatibility.h"

// (insert loyal pledge to kick the person who made windows
// headers as garbage as they are here)
#ifdef _WIN32
	#ifdef SendMessage
		#undef SendMessage
	#endif
#endif

// whatever I give up - fully define WebSocketClient here
#include "WebSocketClient.h"

namespace collab3::proto::http {

	// forward declare further internals
	namespace detail {
		struct Listener;
	}

	/**
	 * WebSockets and HTTP server.
	 */
	struct Server : public std::enable_shared_from_this<Server> {
		friend struct detail::Listener;
		friend struct WebSocketClient;

		explicit Server(net::io_context& ioc);

		~Server();

		// types for handlers
		// (We may have to move these to a hypothetical WebSocket route object.)

		/**
		 * Validate handler.
		 *
		 * This handler gets called during the WebSockets handshake.
		 * Depending on what the handler returns, the connection will progress, or be closed forcefully.
		 *
		 * True == Connection is allowed, handshake continues, eventually Open() is called
		 * False == Connection denied, closed.
		 */
		using Validate_t = std::function<bool(std::weak_ptr<WebSocketClient>)>;
		using Open_t = std::function<void(std::weak_ptr<WebSocketClient>)>;
		using Close_t = std::function<void(std::weak_ptr<WebSocketClient>)>;
		using Message_t = std::function<void(std::weak_ptr<WebSocketClient>, std::shared_ptr<const WebSocketMessage>)>;
		// TODO: Http_t ?
		//  that'd imply needing to export some http session stuff though..

		// TODO: HTTP routing + maybe middleware?.

		void Run(const std::string& host, std::uint16_t port);
		void Stop();

		void SetValidate(Validate_t&&);
		void SetOpen(Open_t&&);
		void SetClose(Close_t&&);
		void SetMessage(Message_t&&);

		[[deprecated("Use the WebSocket client API, and eat exceptions yourself. Will be removed in v3.0.0")]] bool
		SendMessage(std::weak_ptr<WebSocketClient> client, std::shared_ptr<const WebSocketMessage> message);

	   protected:
		Validate_t validate_handler;
		Open_t open_handler;
		Close_t close_handler;
		Message_t message_handler;

	   private:
		net::io_context& ioc;
		std::shared_ptr<detail::Listener> listener;
	};

} // namespace collab3::proto::http

#endif //PROTO_HTTP_SERVER_H
