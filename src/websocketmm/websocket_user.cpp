#include <websocketmm/websocket_user.h>
#include <websocketmm/server.h>

#include <utility>

namespace websocketmm {

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

	// private API fun

	void websocket_user::run(http::request<http::string_body> upgrade) {
		upgrade_request_ = upgrade;

		// Set suggested timeout settings for the websocket
		ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

		ws_.set_option(websocket::stream_base::decorator([&](websocket::response_type& res) {
			res.set(http::field::server, "collab-vm-server/1.2.11");

			// If a subprotocol has been negotiated,
			// we decorate the response with it.
			// Fixes Chrome, which expects this header to exist in the response..

			if(selected_subprotocol_.has_value())
				res.set(http::field::sec_websocket_protocol, selected_subprotocol_.value());
		}));

		// Enable permessage deflate
		websocket::permessage_deflate deflate;
		deflate.server_enable = true;
		ws_.set_option(deflate);

		// We wrap validation and accepting inside of the strand we were given by the
		// listener object to avoid concurrency issues.
		net::post(ws_.get_executor(), [self = shared_from_this()]() {
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

			// Once the verify is finished, we
			// begin accepting the WebSocket connection.
			self->ws_.async_accept(self->upgrade_request_.value(), beast::bind_front_handler(&websocket_user::on_accept, self->shared_from_this()));
		});
	}

	void websocket_user::on_accept(beast::error_code ec) {
		if(ec)
			return;

		// Deallocate the selected subprotocol; Beast internally doesn't care about it once
		// the accept is finished or errored (i.e: when this completion handler is called)
		selected_subprotocol_.reset();

		net::post(ws_.get_executor(), [self = shared_from_this()]() {
			self->server_->open(self);

			// Start looping by reading a message
			self->ws_.async_read(self->buffer_, beast::bind_front_handler(&websocket_user::on_read, self->shared_from_this()));
		});
	}

	void websocket_user::on_read(beast::error_code ec, std::size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);

		if(ec == websocket::error::closed) {
			net::post(ws_.get_executor(), [self = shared_from_this()]() {
				self->server_->close(self->weak_from_this());
			});
			return;
		}

		net::post(ws_.get_executor(), [self = shared_from_this()]() {
			auto type = websocket_message::type::text;

			if(self->ws_.binary())
				type = websocket_message::type::binary;

			auto buffer_size = self->buffer_.size();

			// Call the server's message handler
			// with a temporary built websocket message from our data buffer (The handler can increase lifetime of this data, we don't care once we post it).
			// Once the message handler returns, we consume the buffer.
			self->server_->message(self->weak_from_this(), BuildWebsocketMessage(type, (std::uint8_t*)self->buffer_.data().data(), buffer_size));
			self->buffer_.consume(buffer_size);

			// Queue up another read operation.
			self->ws_.async_read(self->buffer_, beast::bind_front_handler(&websocket_user::on_read, self->shared_from_this()));
		});
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
			return;
		}

		message_queue_.erase(message_queue_.begin());

		// Write more messages to empty the queue
		if(!message_queue_.empty())
			write_message(message_queue_.front());
	}

	void websocket_user::close() {
		net::post(ws_.get_executor(), [self = shared_from_this()]() {
			self->ws_.async_close(websocket::close_reason(websocket::close_code::normal), beast::bind_front_handler(&websocket_user::on_close, self->shared_from_this()));

			// Indicate that we are closing the connection.
			self->closing_ = true;
		});
	}

	void websocket_user::on_close(beast::error_code ec) {
		boost::ignore_unused(ec);
	}

} // namespace websocketmm
