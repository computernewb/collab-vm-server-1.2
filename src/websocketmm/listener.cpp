//
// Created by lily on 1/25/21.
//

#include <websocketmm/listener.h>
#include <websocketmm/server.h>
#include <websocketmm/websocket_user.h>

#include <iostream>
#include <utility>

namespace websocketmm {

	// TODO: Move this to a seperate file, and make it work with POSTs
	// (so we can get the agent to work???) and serve files statically

	struct session : public std::enable_shared_from_this<session> {
		beast::tcp_stream stream_;
		beast::flat_buffer buffer_;
		http::request<http::string_body> req_;
		const std::shared_ptr<server>& server_;

		explicit session(tcp::socket&& socket, const std::shared_ptr<server>& server)
			: stream_(std::move(socket)),
			  server_(server) {
		}

		void run() {
			net::dispatch(stream_.get_executor(),
						  beast::bind_front_handler(
						  &session::do_read,
						  shared_from_this()));
		}

		void do_read() {
			// Make the request empty before reading,
			// otherwise the operation behavior is undefined.
			req_ = {};

			// Set the timeout.
			stream_.expires_after(std::chrono::seconds(2));

			// Read a request
			http::async_read(stream_, buffer_, req_,
							 beast::bind_front_handler(
							 &session::on_read,
							 shared_from_this()));
		}

		void on_read(beast::error_code ec, std::size_t bytes_transferred) {
			boost::ignore_unused(bytes_transferred);

			// This means they closed the connection
			if(ec == http::error::end_of_stream)
				return do_close();

			// Spawn a websocket connection,
			// or close
			if(websocket::is_upgrade(req_)) {
				std::make_shared<websocket_user>(server_, std::move(stream_.release_socket()))->run(req_);
			} else {
				// TODO: process request in this case, for either static server or POST callback
				return do_close();
			}

			//if(ec)
			//   return fail(ec, "read");

			// Send the response
			//handle_request(*doc_root_, std::move(req_), lambda_);
		}

		void on_write(bool close, beast::error_code ec, std::size_t bytes_transferred) {
			boost::ignore_unused(bytes_transferred);

			//if(ec)
			//   return fail(ec, "write");

			if(close) {
				// This means we should close the connection, usually because
				// the response indicated the "Connection: close" semantic.
				return do_close();
			}

			// We're done with the response so delete it
			//res_ = nullptr;

			// Read another request
			do_read();
		}

		void do_close() {
			// Send a TCP shutdown
			beast::error_code ec;
			stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

			// At this point the connection is closed gracefully.
		}
	};

	listener::listener(net::io_context& ioc, tcp::endpoint ep, const std::shared_ptr<server>& server)
		: ioc_(ioc),
		  acceptor_(ioc),
		  endpoint(std::move(ep)),
		  server_(server) {
	}

	void listener::start() {
		beast::error_code ec;

		// Open the acceptor
		acceptor_.open(endpoint.protocol(), ec);
		if(ec) {
			return;
		}

		// Allow address reuse
		acceptor_.set_option(net::socket_base::reuse_address(true), ec);
		if(ec) {
			return;
		}

		// Bind to the server address
		acceptor_.bind(endpoint, ec);
		if(ec) {
			return;
		}

		// Start listening for connections
		acceptor_.listen(net::socket_base::max_listen_connections, ec);
		if(ec) {
			return;
		}

		acceptor_.async_accept(net::make_strand(ioc_), beast::bind_front_handler(&listener::on_accept, shared_from_this()));
	}

	void listener::stop() {
		boost::system::error_code ec;
		acceptor_.cancel(ec);
	}

	void listener::on_accept(beast::error_code ec, tcp::socket socket) {
		if(ec) {
			//std::cout << "Error Code And Of : " << ec.message() << '\n';
			return;
		}

		std::make_shared<session>(std::move(socket), server_)->run();

		// Do another connection
		acceptor_.async_accept(net::make_strand(ioc_),
							   beast::bind_front_handler(&listener::on_accept, shared_from_this()));
	}

} // namespace websocketmm