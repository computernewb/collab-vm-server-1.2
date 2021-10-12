#include <websocketmm/listener.h>
#include <websocketmm/server.h>
#include <websocketmm/websocket_user.h>
#include <websocketmm/http_util.h>
#include <iostream>

#include <utility>
#ifdef _WIN32
#include <sys/stat.h>
#endif
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
			// or process request
			if(websocket::is_upgrade(req_)) {
				std::make_shared<websocket_user>(server_, std::move(stream_.release_socket()))->run(req_);
			} else {
				std::cout << "[websocketmm] Processing HTTP request for " << req_.target() << std::endl;
				if(req_.target().empty() || req_.target()[0] != '/' || req_.target().find("..") != beast::string_view::npos) {
					// alright sorry you can relax it
					return do_close();
				}

				switch(req_.method()) {
					case http::verb::get: {
						std::string path = path_cat("http", req_.target()); // TODO: figure out how to grab argv[2] if it exists
						if(req_.target().back() == '/')
							path.append("index.html");

						struct stat s;
						int st = stat(path.c_str(), &s);
						if(st == 0) {
							if(s.st_mode & S_IFDIR) {
								http::response<http::dynamic_body> response_;
								response_.version(req_.version());
								response_.keep_alive(false);
								response_.set(http::field::server, "collab-vm-server/1.2.11");
								response_.result(http::status::permanent_redirect);
								response_.set(http::field::location, req_.target().to_string() + "/index.html");
								http::write(stream_, response_);
							} else {
								http::file_body::value_type body;
								body.open(path.c_str(), beast::file_mode::scan, ec);
								if(ec) {
									http::response<http::dynamic_body> response_;
									response_.version(req_.version());
									response_.keep_alive(false);
									response_.set(http::field::server, "collab-vm-server/1.2.11");
									response_.result(http::status::internal_server_error);
									beast::ostream(response_.body()) << "Internal server error\r\n";
									response_.content_length(response_.body().size());
									response_.set(http::field::content_type, "text/plain");
									http::write(stream_, response_);
								} else {
									http::response<http::file_body> response_ {
										std::piecewise_construct,
										std::make_tuple(std::move(body)),
										std::make_tuple(http::status::ok, req_.version())
									};
									response_.set(http::field::content_type, mime_type(path));
									response_.result(http::status::ok);
									response_.content_length(response_.body().size());
									http::write(stream_, response_);
								}
							}
						} else {
							http::response<http::dynamic_body> response_;
							response_.version(req_.version());
							response_.keep_alive(false);
							response_.set(http::field::server, "collab-vm-server/1.2.11");
							response_.result(http::status::not_found);
							beast::ostream(response_.body()) << "The resource requested was not found\r\n";
							response_.content_length(response_.body().size());
							response_.set(http::field::content_type, "text/plain");
							http::write(stream_, response_);
						}
						break;
					}
					default:
						return do_close();
				}
			}
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
			// Send a TCP shutdown to the connection.

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
		if(ec)
			return;

		// Allow address reuse
		acceptor_.set_option(net::socket_base::reuse_address(true), ec);
		if(ec)
			return;

		// Bind to the server address
		acceptor_.bind(endpoint, ec);
		if(ec)
			return;

		// Start listening for connections
		acceptor_.listen(net::socket_base::max_listen_connections, ec);
		if(ec)
			return;

		acceptor_.async_accept(net::make_strand(ioc_), beast::bind_front_handler(&listener::on_accept, shared_from_this()));
	}

	void listener::stop() {
		boost::system::error_code ec;
		acceptor_.cancel(ec);
	}

	void listener::on_accept(beast::error_code ec, tcp::socket socket) {
		if(ec)
			return;

		std::make_shared<session>(std::move(socket), server_)->run();

		// Accept another connection
		acceptor_.async_accept(net::make_strand(ioc_),
							   beast::bind_front_handler(&listener::on_accept, shared_from_this()));
	}

} // namespace websocketmm