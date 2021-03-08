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
	 * Build a websocket message to Send to users.
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

	/**
	 * A Websocket connection
	 */
	struct websocket_user : public std::enable_shared_from_this<websocket_user> {
		friend struct listener;
		friend struct session;

		websocket_user(std::shared_ptr<server> server, tcp::socket&& socket);

		per_user_data& GetUserData();

		/**
         * Send a message to this specific user.
         * If an message send for this user is occurring at this point then
         * the message is put in a queue and will be deferred
         *
         * \param[in] message
         */
		void Send(const std::shared_ptr<const websocket_message>& message);

		/**
		 * Get the value of "Sec-Websocket-Protocol" stored in the handshake HTTP request.
		 * This function only returns well-defined results during the verification stage of the connection.
		 */
		beast::string_view GetSubprotocols();

		std::optional<http::request<http::string_body>>& GetUpgradeRequest();

		/**
		 * Select a subprotocol for this user's connection.
		 * Once this function is called, an internal optional is written to
		 * to allow the accept stage of the connection to set the Sec-Websocket-Protocol header to this value.
		 *
		 * \param[in] subprotocol The selected subprotocol
		 */
		void SelectSubprotocol(const std::string& subprotocol);

		inline tcp::socket& socket() {
			return ws_.next_layer().socket();
		}

		/**
		 * Close the WebSocket connection.
		 * This function also clears the send queue for this connection entirely,
		 * and allows this websocket_user to be destroyed.
		 */
		void close();

	   protected:
		//template<class Body>
		void run(http::request<http::string_body> upgrade);

	   private:
		void on_accept(beast::error_code ec);
		void on_read(beast::error_code ec, std::size_t bytes_transferred);
		void on_write(beast::error_code ec, std::size_t bytes_transferred);

		void on_send(const std::shared_ptr<const websocket_message>& message);

		void on_close(beast::error_code ec);

		void write_message(const std::shared_ptr<const websocket_message>& message);

		std::shared_ptr<server> server_;
		per_user_data user_data_;

		beast::flat_buffer buffer_;
		websocket::stream<beast::tcp_stream> ws_;

		/**
		 * A optional containing the HTTP request that spawned the session.
		 * Only valid during validation period.
		 */
		std::optional<http::request<http::string_body>> upgrade_request_;

		std::optional<std::string> selected_subprotocol_;

		/**
		 * Internal value for
		 */
		bool closing_{false};

		/**
		 * internal queue of websocket messages
		 */
		std::vector<std::shared_ptr<const websocket_message>> message_queue_;
	};
} // namespace websocketmm

#endif //WEBSOCKETMM_WEBSOCKET_USER_H
