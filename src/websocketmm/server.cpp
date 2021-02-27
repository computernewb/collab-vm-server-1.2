//
// Created by lily on 1/25/21.
//

#include <websocketmm/server.h>
#include <websocketmm/listener.h>
#include <websocketmm/websocket_user.h>

#include <mutex>

namespace websocketmm {

	server::server(net::io_context& context)
		: ioc_(context) {
	}

	void server::start(const std::string& host, const std::uint16_t port) {
		listener_ = std::make_shared<listener>(ioc_, tcp::endpoint { net::ip::make_address(host), port }, shared_from_this());
		listener_->start();
	}

	void server::stop() {
		// TODO
		listener_->stop();
		listener_.reset();
	}

	/*
	void server::join_to_server(websocket_user* user) {
		std::lock_guard<std::mutex> lock(users_lock_);
		users_.insert(user);
		//open(user);
	}

	void server::leave_server(websocket_user* user) {
		std::lock_guard<std::mutex> lock(users_lock_);
		users_.erase(user);
		//close(user);
	}
	*/

	bool server::verify(const std::weak_ptr<websocketmm::websocket_user>& user) {
		if(verify_handler)
			return verify_handler(user);

		// If there's no attached verify handler, then do not discriminate
		// and allow any connection through
		return true;
	}

	void server::open(const std::weak_ptr<websocketmm::websocket_user>& user) {
		if(open_handler)
			open_handler(user);
	}

	void server::message(const std::weak_ptr<websocketmm::websocket_user>& user, std::shared_ptr<const websocket_message> message) {
		if(message_handler)
			message_handler(user, message);
	}

	void server::close(const std::weak_ptr<websocketmm::websocket_user>& user) {
		if(close_handler)
			close_handler(user);
	}

	bool server::send_message(std::weak_ptr<websocketmm::websocket_user>& user,const std::shared_ptr<const websocket_message>& message) {

		// If the user is expired,
		// don't bother.
		if(user.expired())
			return false;

		// Similar if the message is null.
		if(!message)
			return false;

		if(auto user_sp = user.lock()) {
			user_sp->Send(message);
			return true;
		}

		// If the user shared-ptr somehow
		return false;
	}

	/*
	void server::broadcast_message(const websocket_message& message) {
		// Copy the message into a shared_ptr
		// so we can manage the message lifetime ourselves.
		auto sp = std::make_shared<const websocket_message>(std::move(message));

		// Locally create a list of all connected websocket_user's
		// so we don't have to lock the list
		std::vector<std::weak_ptr<websocket_user>> users;
		{
			std::lock_guard<std::mutex> lock(users_lock_);
			for(auto& user : users_)
				users.emplace_back(user->weak_from_this());
		}

		// Either send the message or add it the user's queue.
		// It will be deleted once the user fully sends it.
		for(const auto& user : users)
			if(auto strong_ptr = user.lock())
				strong_ptr->send(sp);
	}
	 */
} // namespace websocketmm