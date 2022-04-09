//
// CollabVM Server
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#ifndef PIXELBOARD_SERVER_H
#define PIXELBOARD_SERVER_H

#include <boost/asio/io_context.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

// to pull in some stuff we don't wanna
#include <websocket/ForwardDeclarations.h>

#include <websocket/NetworkingTSCompatibility.h>

// (insert loyal pledge to kick the person who made windows
// headers as garbage as they are here)
#ifdef _WIN32
	#ifdef SendMessage
		#undef SendMessage
	#endif
#endif

// whatever I give up - fully define Client here
#include <websocket/Client.h>

namespace collabvm::websocket {

	// forward declare further internals
	namespace detail {
		struct Listener;
	}

	struct Server : public std::enable_shared_from_this<Server> {
		friend struct detail::Listener;
		friend struct Client;

		explicit Server(net::io_context& ioc);

		~Server();

		// types for handlers.
		using Validate_t = std::function<bool(std::weak_ptr<Client>)>; // true == allow connection, false == close.
		using Open_t = std::function<void(std::weak_ptr<Client>)>;
		using Close_t = std::function<void(std::weak_ptr<Client>)>;
		using Message_t = std::function<void(std::weak_ptr<Client>, std::shared_ptr<const Message>)>;
		// TODO: Http_t ?
		//  that'd imply needing to export some http session stuff though..

		void Run(const std::string& host, std::uint16_t port);
		void Stop();

		void SetValidate(Validate_t&&);
		void SetOpen(Open_t&&);
		void SetClose(Close_t&&);
		void SetMessage(Message_t&&);

		bool SendMessage(std::weak_ptr<Client> client, std::shared_ptr<const Message> message);

	   protected:
		Validate_t validate_handler;
		Open_t open_handler;
		Close_t close_handler;
		Message_t message_handler;

	   private:
		net::io_context& ioc;
		std::shared_ptr<detail::Listener> listener;
	};

} // namespace collabvm::http

#endif //PIXELBOARD_SERVER_H
