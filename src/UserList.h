#pragma once
#include <string>
#include <mutex>
#include "CollabVMUser.h"

class UserList {
   public:
	UserList()
		: users_(nullptr),
		  connected_users_(0) {
	}

	template<typename F>
	void AddUser(CollabVMUser& user, F func) {
		std::lock_guard<std::mutex> lock(users_lock_);

		/* Add to list if join was successful */
		user.prev_ = nullptr;
		user.next_ = users_;

		if(users_ != nullptr)
			users_->prev_ = &user;

		users_ = &user;
		connected_users_++;

		func(user);
	}

	template<typename F>
	void RemoveUser(CollabVMUser& user, F func) {
		std::lock_guard<std::mutex> lock(users_lock_);

		/* Update prev / head */
		if(user.prev_ != nullptr)
			user.prev_->next_ = user.next_;
		else
			users_ = user.next_;

		/* Update next */
		if(user.next_ != nullptr)
			user.next_->prev_ = user.prev_;

		connected_users_--;

		func(user);
	}

	/**
	 * Iterate over each user in the list.
	 */
	template<typename F>
	inline void ForEachUser(F func) const {
		CollabVMUser* user = users_;
		while(user) {
			func(*user);
			user = user->next_;
		}
	}

	/**
	 * Lock the users list and iterate over each user in it.
	 */
	template<typename F>
	inline void ForEachUserLock(F func) {
		std::lock_guard<std::mutex> lock(users_lock_);
		ForEachUser(func);
	}

   private:
	/**
	* The first user within the list of all connected users, or NULL if no
	* users are currently connected.
	*/
	CollabVMUser* users_;

	/**
	* Lock which is acquired when the users list is being manipulated.
	*/
	std::mutex users_lock_;

	size_t connected_users_;
};
