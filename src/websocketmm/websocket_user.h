//
// Created by lily on 1/25/21.
//

#ifndef WEBSOCKETMM_WEBSOCKET_USER_H
#define WEBSOCKETMM_WEBSOCKET_USER_H

#include <websocketmm/beast/net.h>
#include <websocketmm/beast/beast.h>

#include <cstdint>
#include <memory>
#include <vector>
#include <optional> // wow, we can use this now!

// forward decl in thing
struct CollabVMUser;

namespace websocketmm {

	struct websocket_message {
		enum class type : std::uint8_t {
			text,
			binary
		};

		type message_type;
		std::vector<std::uint8_t> data;
	};

	/**
     * Build a websocket message to send.
     *
     */
	std::shared_ptr<const websocket_message> BuildWebsocketMessage(websocket_message::type t, std::uint8_t* data, std::size_t size);
	std::shared_ptr<const websocket_message> BuildWebsocketMessage(const std::string& str);

	struct server;

	/**
    * per user data type
    */
	struct per_user_data {
		// TODO: use std::any
		std::shared_ptr<CollabVMUser> user;
	};

	struct websocket_user : public std::enable_shared_from_this<websocket_user> {
		friend struct listener;
		friend struct session;

		websocket_user(std::shared_ptr<server> server, tcp::socket&& socket);
		//~websocket_user();

		per_user_data& get_userdata();

		/**
         * send a message. If message send ops are occuring at this point then
         * the message is put in a queue
         * \param[in] message
         */
		void send(const std::shared_ptr<const websocket_message>& message);

		inline beast::string_view get_subprotocols() { // TODO:.cpp
			if(upgrade_request_.has_value())
				return upgrade_request_.value()[http::field::sec_websocket_protocol];

			return "";
		}

		void select_subprotocol(const std::string& subprotocol);

		inline const tcp::socket& socket() {
			return ws_.next_layer().socket();
		}

		void close();

	   protected:
		//template<class Body>
		void run(http::request<http::string_body> upgrade);

	   private:
		void on_accept(beast::error_code ec);
		void on_read(beast::error_code ec, std::size_t bytes_transferred);
		void on_write(beast::error_code ec, std::size_t bytes_transferred);

		void on_send(const std::shared_ptr<const websocket_message>& message);

		void write_message(const std::shared_ptr<const websocket_message>& message);

		/**
		 * a wrapper for server->Validate()
		 * because we manage header dependencies properly
		 */
		bool do_validate();

		std::shared_ptr<server> server_;
		per_user_data user_data_;

		beast::flat_buffer buffer_;
		websocket::stream<beast::tcp_stream> ws_;

		/**
		 * http request
		 * only valid during validation period
		 */
		std::optional<http::request<http::string_body>> upgrade_request_;

		std::optional<std::string> selected_subprotocol_;

		/**
		 * internal queue of websocket messages
		 */
		std::vector<std::shared_ptr<const websocket_message>> message_queue_;
		std::mutex message_queue_lock_;

	};
} // namespace websocketmm

#endif //COLLAB_VM_SERVER_CAPTCHA_WEBSOCKET_USER_H
