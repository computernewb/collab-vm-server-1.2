#ifndef WEBSOCKETMM_LISTENER_H
#define WEBSOCKETMM_LISTENER_H

#include <websocketmm/beast/net.h>
#include <websocketmm/beast/beast.h>
#include <memory>

namespace websocketmm {
	struct server;

	struct listener : public std::enable_shared_from_this<listener> {
		listener(net::io_context& ioc, tcp::endpoint ep, const std::shared_ptr<server>& server);

		void start();
		void stop();

	   private:
		void on_accept(beast::error_code ec, tcp::socket socket);

		net::io_context& ioc_;
		tcp::acceptor acceptor_;
		tcp::endpoint endpoint;

		std::shared_ptr<server> server_;
	};

} // namespace websocketmm

#endif //WEBSOCKETMM_LISTENER_H
