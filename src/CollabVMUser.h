#pragma once
#include <array>
#include <memory>
#include <map>
#include <fstream>
#include <stdint.h>
#include "GuacUser.h"

#include <websocketmm/fwd.h>

class VMController;
struct UploadInfo;

enum UserRank : uint8_t {
	kUnregistered, // Guest
	kRegistered,   // Registered account
	kAdmin,		   // Administrator
	kModerator	   // Moderator
};

enum UserMuted : uint8_t {
	kUnmuted,  // Not muted
	kTempMute, // Muted for time in Admin Panel
	kPermMute  // Indefinite mute
};

/**
 * All of the data associated with an IP address used for
 * spam prevention.
 */
struct IPData {
	virtual ~IPData() = default;

	/**
	 * The current number of connections from the IP.
	 */
	uint8_t connections;

	/**
	 * The last time a chat message was sent from this IP.
	 */
	std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds> last_chat_msg;

	/**
	 * The number of chat messages that were sent within a certain amount of time.
	 */
	uint8_t chat_msg_count;

	/**
	 * Whether the user is muted from the chat.
	 */
	UserMuted chat_muted;


	std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds> last_name_chg;

	uint8_t name_chg_count;

	bool name_fixed;

	std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds> last_turn;

	uint8_t turn_count;

	bool turn_fixed;

	std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds> last_connect;

	uint8_t connect_count;

	bool connect_fixed;

	enum class VoteDecision : uint_fast8_t {
		kNotVoted,
		kYes,
		kNo
	};

	std::map<const VMController*, VoteDecision> votes;

	/**
	 * The number of failed login attempts to the admin panel.
	 */
	uint8_t failed_logins;

	/**
	 * The time of the first failed login from the IP.
	 */
	std::chrono::time_point<std::chrono::steady_clock, std::chrono::minutes> failed_login_time;

	virtual std::string GetIP() const = 0;

	enum class IPType { kIPv4,
						kIPv6 } type;

   protected:
	IPData(IPType type, bool one_connection)
		: type(type),
		  connections(one_connection),
		  chat_msg_count(0),
		  name_fixed(false),
		  name_chg_count(0),
		  chat_muted(kUnmuted),
		  //has_voted(false),
		  failed_logins(0) {
	}
};

struct IPv4Data : IPData {
	IPv4Data(uint32_t addr, bool one_connection)
		: IPData(IPType::kIPv4, one_connection),
		  addr(addr) {
	}

	virtual std::string GetIP() const {
		char str[16];
		int len = snprintf(str, sizeof(str), "%hhu.%hhu.%hhu.%hhu",
						   static_cast<uint8_t>((addr >> 24) & 0xFF),
						   static_cast<uint8_t>((addr >> 16) & 0xFF),
						   static_cast<uint8_t>((addr >> 8) & 0xFF),
						   static_cast<uint8_t>((addr)&0xFF));
		if(len > 0 && len < sizeof(str))
			return std::string(str, len);
		else
			return std::string();
	}

	uint32_t addr;
};

struct IPv6Data : IPData {
	IPv6Data(const std::array<uint8_t, 16>& addr, bool one_connection)
		: IPData(IPType::kIPv6, one_connection),
		  addr(addr) {
	}

	virtual std::string GetIP() const {
		// TODO: use shorter notation when possible
		char str[40];
		const uint8_t* x = addr.data();
		int len = snprintf(str, sizeof(str),
						   "%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:"
						   "%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx",
						   x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7],
						   x[8], x[9], x[10], x[11], x[12], x[13], x[14], x[15]);
		if(len > 0 && len < sizeof(str))
			return std::string(str, len);
		else
			return std::string();
	}

	std::array<uint8_t, 16> addr;
};

/**
 * Connection data associated with a websocket connection.
 */
class CollabVMUser : public std::enable_shared_from_this<CollabVMUser> {
   public:
	CollabVMUser(std::weak_ptr<websocketmm::websocket_user> handle, IPData& ip_data)
		: next_(nullptr),
		  prev_(nullptr),
		  handle(handle),
		  vm_controller(nullptr),
		  guac_user(nullptr),
		  waiting_turn(false),
		  user_rank(UserRank::kUnregistered),
		  //user_id(0),
		  connected(false),
		  admin_connected(false),
		  last_nop_instr(std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now())),
		  ip_data(ip_data),
		  voted_amount(0),
		  voted_limit(false) {
	}

	// Intrusive list for all of the connections viewing a VM
	CollabVMUser* next_;
	CollabVMUser* prev_;

	/**
	 * The websocket handle associated with the client's
	 * websocket connection.
	 */
	std::weak_ptr<websocketmm::websocket_user> handle;

	/**
	 * The VM controller that the client is viewing. May be null.
	 */
	VMController* vm_controller;

	/**
	 * The Guacamole user associated with the VM that the client
	 * is viewing. May be null if the user is not viewing a VM.
	 */
	GuacUser* guac_user;

	/**
	 * Set to true when the user is waiting for a turn to control the VM.
	 */
	bool waiting_turn;

	/**
	 * The rank of the user.
	 */
	UserRank user_rank;

	/**
	 * The user's ID if they have a registered account.
	 */
	//uint32_t user_id;

	/**
	 * The username of the client. May be empty if a username
	 * has not yet been chosen for or by the user.
	 */
	std::shared_ptr<std::string> username;

	/**
	 * Whether the client is connected to the websocket server.
	 */
	bool connected;

	/**
	 * True when the user is viewing the admin panel and should receive
	 * data for it.
	 */
	bool admin_connected;

	/**
	 * The time that the last nop instruction was received.
	 */
	std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds> last_nop_instr;

	IPData& ip_data;

	/**
	 * How many times the user has voted during a vote.
	 * (TODO: I should move this into IPData possibly)
	 */
	int voted_amount;
	bool voted_limit;
};
