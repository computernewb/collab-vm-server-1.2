//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#include "Server.h"

#include <boost/asio/ip/tcp.hpp>

#include "WebSocketClient.h"

namespace collab3::proto::http {

	std::shared_ptr<detail::Listener> RunListener(net::io_context& ioc, tcp::endpoint&& ep,
												  std::shared_ptr<Server> server);

	Server::Server(net::io_context& ioc) :
		ioc(ioc) {
	}

	Server::~Server() {
		Stop();
	}

	void Server::Run(const std::string& host, std::uint16_t port) {
		listener = RunListener(ioc, tcp::endpoint { net::ip::make_address(host), port }, shared_from_this());
	}

	void Server::Stop() {
		if(listener)
			listener.reset();
	}

	void Server::SetValidate(Validate_t&& validate) {
		validate_handler = std::move(validate);
	}

	void Server::SetOpen(Open_t&& open) {
		open_handler = std::move(open);
	}

	void Server::SetMessage(Message_t&& message) {
		message_handler = std::move(message);
	}

	void Server::SetClose(Close_t&& close) {
		close_handler = std::move(close);
	}

	bool Server::SendMessage(std::weak_ptr<WebSocketClient> client, std::shared_ptr<const WebSocketMessage> message) {
		try {
			if(auto sp = client.lock()) {
				sp->Send(message);
				return true;
			}
		} catch(...) {
			return false;
		}

		return false;
	}
} // namespace collab3::proto::http