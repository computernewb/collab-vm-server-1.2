//
// Created by lily on 11/20/2021.
//

#include <memory>

// don't need the full definition of Server here.
#include <websocket/NetworkingTSCompatibility.h>
#include <websocket/ForwardDeclarations.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/bind_handler.hpp>

// forward decl of http session for prototype
namespace collabvm::websocket { // NOLINT
	namespace detail {
		struct HttpSession;
	}
} // namespace collabvm::http

namespace collabvm::websocket {

	std::shared_ptr<detail::HttpSession> RunHttpSession(tcp::socket&& socket, std::shared_ptr<Server> server);

	namespace detail {

		struct Listener : public std::enable_shared_from_this<Listener> {
			Listener(net::io_context& ioc, tcp::endpoint&& ep, std::shared_ptr<Server> server)
				: ioc(ioc),
				  acceptor(ioc),
				  endpoint(std::move(ep)),
				  server(server) {
			}

			~Listener() {
				Stop();
			}

			void Run() {
				boost::system::error_code ec;

				acceptor.open(endpoint.protocol(), ec);
				if(ec)
					return;

				acceptor.set_option(net::socket_base::reuse_address(true), ec);
				if(ec)
					return;

				acceptor.bind(endpoint, ec);
				if(ec)
					return;

				acceptor.listen(net::socket_base::max_listen_connections, ec);
				if(ec)
					return;

				// Begin the accept loop.
				AcceptOne();
			}

			void Stop() {
				boost::system::error_code ec;
				acceptor.cancel(ec);
			}

		   private:
			void AcceptOne() {
				acceptor.async_accept(net::make_strand(ioc), boost::beast::bind_front_handler(&Listener::OnAccept, shared_from_this()));
			}

			void OnAccept(const boost::system::error_code& ec, tcp::socket socket) {
				if(ec != net::error::operation_aborted) {
					// Start a HTTP session for this user.
					// The HTTP session will create a WebSocket session
					// and then destroy itself if need be.
					RunHttpSession(std::move(socket), server);

					// Accept another connection, if we aren't
					// being called thanks to a cancellation request.
					AcceptOne();
				}
			}

			net::io_context& ioc;

			tcp::acceptor acceptor;
			tcp::endpoint endpoint;

			std::shared_ptr<Server> server;
		};

	} // namespace detail

	std::shared_ptr<detail::Listener> RunListener(net::io_context& ioc, tcp::endpoint&& ep, std::shared_ptr<Server> server) {
		auto sp = std::make_shared<detail::Listener>(ioc, std::move(ep), server);
		sp->Run();
		return sp;
	}

} // namespace collabvm::http