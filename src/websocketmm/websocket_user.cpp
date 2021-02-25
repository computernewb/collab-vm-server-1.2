//
// Created by lily on 1/25/21.
//

#include <websocketmm/websocket_user.h>
#include <websocketmm/server.h>

#include <iostream>

namespace websocketmm {

	std::shared_ptr<const websocket_message> BuildWebsocketMessage(websocket_message::type t, std::uint8_t* data, std::size_t size) {
		auto m = std::make_shared<websocket_message>();

		// copy the data in
		m->data.resize(size);
		std::memcpy(&m->data[0], &data[0], size);

		return m;
	}

	std::shared_ptr<const websocket_message> BuildWebsocketMessage(const std::string& str) {
		return BuildWebsocketMessage(websocket_message::type::text, (std::uint8_t*)str.data(), str.length());
	}

	websocket_user::websocket_user(std::shared_ptr<server> server, tcp::socket&& socket)
		: server_(server),
		  ws_(std::move(socket)) {
	}

	websocket_user::~websocket_user() {
		server_->leave_server(this);
		//std::cout << "~websocket_user() @ " << this << '\n';
		//server_->close(this);
	}

	bool websocket_user::do_validate() {
		return server_->verify(this);
	}

	per_user_data& websocket_user::get_userdata() {
		return user_data_;
	}

	void websocket_user::run(http::request<http::string_body> upgrade) {
		upgrade_request_ = upgrade;

		// Set suggested timeout settings for the websocket
		ws_.set_option(
		websocket::stream_base::timeout::suggested(
		beast::role_type::server));

		// Ploy as WebSocket++.
		ws_.set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
			res.set(http::field::server,
					"WebSocket++/0.8.1");
		}));

		if(!do_validate()) {
			// Validation failed. Close the connection and return early,
			// the shared_ptr destructor will clean us up
			ws_.next_layer().socket().close();
			return;
		}

		// Once the verify is finished, we
		// clear the upgrade request.
		upgrade_request_.reset();

		ws_.async_accept(
		upgrade,
		beast::bind_front_handler(
		&websocket_user::on_accept,
		shared_from_this()));
	}

	void websocket_user::on_accept(beast::error_code ec) {
		if(ec)
			return;

		server_->open(this);
		server_->join_to_server(this);

		// Start looping by reading a message
		ws_.async_read(
		buffer_,
		beast::bind_front_handler(
		&websocket_user::on_read,
		shared_from_this()));
	}

	void websocket_user::on_read(beast::error_code ec, std::size_t bytes_transferred) {
		if(ec == websocket::error::closed) {
			server_->close(this);
			return;
		}

		auto type = websocket_message::type::text;

		if(ws_.binary())
			type = websocket_message::type::binary;

		// Build the message.
		auto message = BuildWebsocketMessage(type, (std::uint8_t*)buffer_.data().data(), buffer_.size());

		buffer_.consume(buffer_.size());

		// call the message handler
		server_->message(this, message);

		// then read anothe rmessage
		ws_.async_read(
		buffer_,
		beast::bind_front_handler(
		&websocket_user::on_read,
		shared_from_this()));
	}

	void websocket_user::send(const std::shared_ptr<const websocket_message>& message) {
		// Post the work to happen on the strand,
		// to avoid concurrency problems.
		net::post(
		ws_.get_executor(),
		beast::bind_front_handler(
		&websocket_user::on_send,
		shared_from_this(),
		message));
	}

	void websocket_user::on_send(const std::shared_ptr<const websocket_message>& message) {
		// Always add to queue
		message_queue_.push_back(message);

		// Are we already writing?
		if(message_queue_.size() > 1)
			return;

		write_message(message_queue_.front());
	}

	void websocket_user::write_message(const std::shared_ptr<const websocket_message>& message) {
		ws_.binary(message->message_type == websocket_message::type::binary);

		ws_.async_write(
		net::buffer(message->data),
		beast::bind_front_handler(
		&websocket_user::on_write,
		shared_from_this()));
	}

	void websocket_user::on_write(beast::error_code ec, std::size_t bytes_transferred) {
		if(ec == websocket::error::closed) {
			server_->close(this);
			return;
		}

		if(ec)
			return;

		message_queue_.erase(message_queue_.begin());

		if(!message_queue_.empty())
			// Write more messages then
			write_message(message_queue_.front());
	}

	void websocket_user::close() {
		beast::error_code ec;
		ws_.close(websocket::close_reason(websocket::close_code::normal), ec);
		//server_->close(this);
	}

} // namespace websocketmm
