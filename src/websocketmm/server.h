//
// Created by lily on 1/24/21.
//

#ifndef WEBSOCKETMM_SERVER_H
#define WEBSOCKETMM_SERVER_H

#include <string>
#include <cstdint>
#include <set>
#include <memory>
#include <functional>

#include <websocketmm/beast/net.h>

namespace websocketmm {

	// forward declarations
	struct listener;
	struct websocket_user;

	struct websocket_message;

	struct server : public std::enable_shared_from_this<server> {
		friend struct websocket_user;
		friend struct listener;

		explicit server(net::io_context& context_);

		void start(const std::string& host, const std::uint16_t port);

		void stop();

		/**
         * Broadcast a message to all connected Websocket users.
         *
         * \param[in] message Message to broadcast
         */
		void broadcast_message(const websocket_message& message);

		inline void set_verify_handler(std::function<bool(websocket_user*)> handler) {
			verify_handler = handler;
		}

		inline void set_open_handler(std::function<void(websocket_user*)> handler) {
			open_handler = handler;
		}

		inline void set_message_handler(std::function<void(websocket_user*, std::shared_ptr<const websocket_message>)> handler) {
			message_handler = handler;
		}

		inline void set_close_handler(std::function<void(websocket_user*)> handler) {
			close_handler = handler;
		}

	   protected:
		void join_to_server(websocket_user* user);
		void leave_server(websocket_user* user);

		// these run the internal handlers or do nothing
		bool verify(websocket_user* user);
		void open(websocket_user* user);
		void message(websocket_user* user, std::shared_ptr<const websocket_message> message);

		void close(websocket_user* user);

	   private:
		/**
         * A reference to the io_context held here.
         */
		net::io_context& ioc_;
		std::shared_ptr<listener> listener_;

		std::mutex users_lock_;
		std::set<websocket_user*> users_;

		// Handlers

		std::function<bool(websocket_user*)> verify_handler;
		std::function<void(websocket_user*)> open_handler;
		std::function<void(websocket_user*, std::shared_ptr<const websocket_message>)> message_handler;
		std::function<void(websocket_user*)> close_handler;
	};

} // namespace websocketmm

#endif //WEBSOCKETMM_SERVER_H
