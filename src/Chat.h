#pragma once
#include <string>
#include <memory>
#include <chrono>
#include "CollabVMUser.h"

struct ChatMessage {
	//ChatMessage(CollabVMUser& user, std::string message) :
	//	username(user.username),
	//	message(message)
	//{
	//}

	std::shared_ptr<std::string> username;
	std::string message;
	std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds> timestamp;
};
