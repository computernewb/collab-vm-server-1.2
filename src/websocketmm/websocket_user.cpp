#include <websocketmm/websocket_user.h>
#include <websocketmm/server.h>

#include <utility>

// Uncomment if you're going to use Websocket-- under a proxy.
//#define WEBSOCKETMM_SUPPORT_PROXYING

namespace websocketmm {

	// TODO(lily): these should be in another TU

	std::shared_ptr<const websocket_message> BuildWebsocketMessage(websocket_message::type t, std::uint8_t* data, std::size_t size) {
		auto m = std::make_shared<websocket_message>();

		m->message_type = t;

		// empty message moment
		if(!data) {
			return m;
		}

		// else copy the data in
		m->data.resize(size);
		std::memcpy(&m->data[0], &data[0], size);
		return m;
	}

	std::shared_ptr<const websocket_message> BuildWebsocketMessage(const std::string& str) {
		return BuildWebsocketMessage(websocket_message::type::text, (std::uint8_t*)str.data(), str.length());
	}

	// TODO utf-16 overloads

	websocket_user::websocket_user(std::shared_ptr<server> server, tcp::socket&& socket)
		: server_(std::move(server)),
		  ws_(std::move(socket)) {
	}

	beast::string_view websocket_user::GetSubprotocols() {
		if(upgrade_request_.has_value())
			return upgrade_request_.value()[http::field::sec_websocket_protocol];

		return "";
	}

	per_user_data& websocket_user::GetUserData() {
		return user_data_;
	}

	std::optional<http::request<http::string_body>>& websocket_user::GetUpgradeRequest() {
		return upgrade_request_;
	}

	void websocket_user::SelectSubprotocol(const std::string& subprotocol) {
		selected_subprotocol_ = subprotocol;
	}

	net::ip::address websocket_user::GetAddress() {
		// return real IP if it's not a proxy client
		if(!proxy_address_.has_value()) {
			const auto address = ws_.next_layer().socket().remote_endpoint().address();
			return address;
		}

		return proxy_address_.value();
	}

	// private API fun

	void websocket_user::run(http::request<http::string_body> upgrade) {
		upgrade_request_ = upgrade;

		// Set suggested timeout settings for the websocket
		// this apparently needs to be done *after* the handshake
		// go figure
		//ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

		// some fancy useragent stuffs
		ws_.set_option(websocket::stream_base::decorator([&](websocket::response_type& res) {
			res.set(http::field::server, "collab-vm-server/1.2.11");

			// If a subprotocol has been negotiated,
			// we decorate the response with it.
			// Fixes Chrome, which expects this header to exist in the response..
			// (which Chrome ironically enough is in the right and follows the spec properly)

			if(selected_subprotocol_.has_value())
				res.set(http::field::sec_websocket_protocol, selected_subprotocol_.value());
		}));

		// Enable permessage deflate
		//		websocket::permessage_deflate deflate;
		//		deflate.server_enable = true;
		//		ws_.set_option(deflate);

		// We wrap validation and accepting inside of the strand we were given by the
		// listener object to avoid concurrency issues.

		net::post(ws_.get_executor(), [self = shared_from_this()]() {

#ifdef WEBSOCKETMM_SUPPORT_PROXYING
			// seperate scope because this stuff isn't really needed after
			{
				auto& req = self->upgrade_request_.value();

				auto& header_val = req["X-Forwarded-For"];

				if(header_val != "") {
					boost::system::error_code ec;
					
					// X-Forwarded-For is actually a tokenized list, like Sec-Websocket-Protocol.
					// (see https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/X-Forwarded-For)
					//
					// However, the first element is ALWAYS the client hop/IP address.
					// All other elements are just the proxy hops the client has been through,
					// which we don't really need to care about. (We only care about the client hop)

					auto len = header_val.find(",");

					auto ip = net::ip::make_address(std::string_view(header_val.data(), len == boost::beast::string_view::npos ? header_val.size() : len), ec);

					if(ec) {
						// There was an error parsing the IP, so as to not compromise security,
						// close the connection.
						beast::error_code b_ec;
						self->socket().shutdown(tcp::socket::shutdown_send, b_ec);
						return;
					} else {
						// Well, it works!
						self->proxy_address_ = ip;
					}
				}
			}
#endif // WEBSOCKETMM_SUPPORT_PROXYING

			if(!self->server_->verify(self->weak_from_this())) {
				// Validation failed.
				// Close the connection and return early.
				//
				// Resources will be cleaned up by the shared_ptr destructor.
				beast::error_code ec;
				self->socket().shutdown(tcp::socket::shutdown_send, ec);

				// connection is gracefully shut down once we reach this point
				return;
			}

			// Once the verify is finished, we begin accepting the WebSocket connection.
			self->ws_.async_accept(self->upgrade_request_.value(), beast::bind_front_handler(&websocket_user::on_accept, self->shared_from_this()));
		});
	}

	void websocket_user::on_accept(beast::error_code ec) {
		if(ec)
			return;

		// Deallocate the selected subprotocol; Beast internally doesn't care about it
		// (and doesn't need to; it's WS-- code) once the accept finishes or errors
		// (i.e: when this completion handler is called)
		selected_subprotocol_.reset();

		net::post(ws_.get_executor(), [self = shared_from_this()]() {
			// Call the defined open callback
			self->server_->open(self);

			// Start the asynchronous loop by reading a message
			self->ws_.async_read(self->buffer_, beast::bind_front_handler(&websocket_user::on_read, self->shared_from_this()));
		});
	}

	void websocket_user::on_read(beast::error_code ec, std::size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);

		if(ec) {
			// Close the connection if a error occurs.
			server_->close(weak_from_this());
			close();
			return;
		}

		// In most cases since we're on the strand already this should be a non-issue,
		// but just in case we aren't, we post the work to avoid any issues

		//net::post(ws_.get_executor(), [self = shared_from_this()]() {

			auto type = websocket_message::type::text;

			if(ws_.binary())
				type = websocket_message::type::binary;

			// Call the server's message handler.
			// BuildWebsocketMessage is used to return a temporary copied buffer which the server code can make last longer if it needs to.

			server_->message(weak_from_this(), BuildWebsocketMessage(type, (std::uint8_t*)buffer_.data().data(), buffer_.size()));
			buffer_.clear();

			// Queue up another read operation.
			ws_.async_read(buffer_, beast::bind_front_handler(&websocket_user::on_read, shared_from_this()));
	}

	void websocket_user::Send(const std::shared_ptr<const websocket_message>& message) {
		// Post the work to happen on the strand to avoid concurrency problems.
		net::post(ws_.get_executor(), beast::bind_front_handler(&websocket_user::on_send, shared_from_this(), message));
	}

	void websocket_user::on_send(const std::shared_ptr<const websocket_message>& message) {
		// If the connection is closing,
		// we immediately return. This is to avoid
		// placing a write operation during closing sequence.
		if(closing_)
			return;

		message_queue_.push_back(message);

		// If we are already trying to write a message,
		// return early so that whatever is writing can do it for us.
		if(message_queue_.size() > 1)
			return;

		// Otherwise, we should write the message immediately.
		write_message(message_queue_.front());
	}

	void websocket_user::write_message(const std::shared_ptr<const websocket_message>& message) {
		// Return immediately if the socket is pending a close,
		// so we can empty the message queue without scheduling writes
		// (which is bad once the closing sequence has begin)
		if(closing_)
			return;

		// Configure the message type and schedule a asynchronous write.
		ws_.binary(message->message_type == websocket_message::type::binary);
		ws_.async_write(net::buffer(message->data), beast::bind_front_handler(&websocket_user::on_write, shared_from_this()));
	}

	void websocket_user::on_write(beast::error_code ec, std::size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);

		if(ec) {
			message_queue_.clear();
			return;
		}

		message_queue_.erase(message_queue_.begin());

		// Write more messages to empty the queue
		if(!message_queue_.empty())
			write_message(message_queue_.front());
	}

	void websocket_user::close() {
		net::post(ws_.get_executor(), [self = shared_from_this()]() {
			// Indicate that we are closing the connection.
			self->closing_ = true;
			self->ws_.async_close(websocket::close_reason(websocket::close_code::normal), beast::bind_front_handler(&websocket_user::on_close, self->shared_from_this()));
		});
	}

	void websocket_user::on_close(beast::error_code ec) {
		boost::ignore_unused(ec);
	}

} // namespace websocketmm
