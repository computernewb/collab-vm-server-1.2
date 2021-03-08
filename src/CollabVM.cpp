/*
Collab VM
Created By:
Cosmic Sans
Dartz
Geodude

Special Thanks:

modeco80
CHOCOLATEMAN
Colonel Seizureton/Colonel Munchkin
CtrlAltDel
FluffyVulpix
hannah
LoveEevee
Matthew
Vanilla

and the rest of the Collab VM Community for all their help over the years, including, but of course not limited to:
Donating, using the site, telling their friends/family, being a great help, and more.

A special shoutout to the Collab VM community for being such a great help. You've made the last 5 years great.

Here is the team's special thanks to you - the Collab VM Server Source Code.
We really hope you enjoy and we hope you continue using the website. Thank you all.
The Webframe source code is here: https://github.com/computernewb/collab-vm-web-app

Please email rightowner@gmail.com for any assistance.

~Cosmic Sans, Dartz, and Geodude
*/

#include "CollabVM.h"
#include "GuacInstructionParser.h"

#include <boost/algorithm/string.hpp>

#include <websocketmm/server.h>
#include <websocketmm/websocket_user.h>

#include "cpr/cpr.h"

#include <memory>
#include <iostream>
#include <fstream>
#ifdef _WIN32
	#include <sys/types.h>
#endif

#include <sys/stat.h>

#include "guacamole/user-handlers.h"
#include "guacamole/unicode.h"
#ifdef USE_JPEG
	#include "guacamole/protocol.h"
#endif

//#include <ossp/uuid.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>

#define STR_LEN(str) sizeof(str) - 1

using rapidjson::Document;
using rapidjson::Value;
using rapidjson::GenericMemberIterator;

const uint8_t CollabVMServer::kIPDataTimerInterval = 1;

enum class ClientFileOp : char {
	kBegin = '0',
	kMiddle = '1',
	kEnd = '2',
	kStop = '3'
};

/*
enum class ServerFileOp : char
{
	kBegin = '0',
	kAck = '1',
	kFinished = '2',
	kStop = '3',
	kWaitTime = '4',
	kFailed = '5',
	kUploadInProgress = '6',
	kTimedOut = '7'
};
*/

// TODO constexpr string view

/**
 * The functions that can be performed from the admin panel.
 */
enum CONFIG_FUNCTIONS {
	kSettings,	  // Update server settings
	kPassword,	  // Change master password
	kModPassword, // Change moderator password
	kAddVM,		  // Add a new VM
	kUpdateVM,	  // Update a VM's settings
	kDeleteVM	  // Delete a VM
};

/**
 * The names of functions that can be performed from the admin panel.
 */
const static std::string config_functions_[] = {
	"settings",
	"password",
	"mod-pw",
	"add-vm",
	"update-vm",
	"del-vm"
};

enum admin_opcodes_ {
	kStop,			  // Stop an admin connection
	kSeshID,		  // Authenticate with a session ID
	kMasterPwd,		  // Authenticate with the master password
	kGetSettings,	  // Get current server settings
	kSetSettings,	  // Update server settings
	kQEMU,			  // Execute QEMU monitor command
	kStartController, // Start one or more VM controllers
	kStopController,  // Stop one or more VM controllers
	kRestoreVM,		  // Restore one or more VMs
	kRebootVM,		  // Reboot one or more VMs
	kResetVM,		  // Reset one or more VMs
	kRestartVM,		  // Restart one or more VM hypervisors
	kBanUser,		  // Ban user's IP address
	kForceVote,		  // Force the results of the Vote for Reset
	kMuteUser,		  // Mute a user
	kKickUser,		  // Forcefully remove a user from a VM (Kick)
	kEndUserTurn,	  // End a user's turn
	kClearTurnQueue,  // End all turns
	kRenameUser,	  // Rename a user
	kUserIP,		  // Sends back a user's IP address
	kForceTakeTurn	  // Skip the queue and forcefully take a turn (Turn-jacking)
};

enum SERVER_SETTINGS {
	kChatRateCount,
	kChatRateTime,
	kChatMuteTime,
	kMaxConnections,
	kMaxUploadTime,
	kBanCommand,
	kJPEGQuality,
	kModEnabled,
	kModPerms,
	kBlacklistedUsernames
};

const static std::string server_settings_[] = {
	"chat-rate-count",
	"chat-rate-time",
	"chat-mute-time",
	"max-cons",
	"max-upload-time",
	"ban-cmd",
	"jpeg-quality",
	"mod-enabled",
	"mod-perms",
	"blacklisted-usernames"
};

enum VM_SETTINGS {
	kName,
	kAutoStart,
	kDisplayName,
	kHypervisor,
	kStatus,
	kRestoreShutdown,
	kRestoreTimeout,
	kRestorePeriodic,
	kPeriodRestoreTime,
	kVNCAddress,
	kVNCPort,
	kQMPSocketType,
	kQMPAddress,
	kQMPPort,
	kMaxAttempts,
	kQEMUCmd,
	kQEMUSnapshotMode,
	kTurnsEnabled,
	kTurnTime,
	kVotesEnabled,
	kVoteTime,
	kVoteCooldownTime,
	kAgentEnabled,
	kAgentSocketType,
	kAgentUseVirtio,
	kAgentAddress,
	kAgentPort,
	kRestoreHeartbeat,
	kHeartbeatTimeout,
	kUploadsEnabled,
	kUploadCooldownTime,
	kUploadMaxSize,
	kUploadMaxFilename,
	kMOTD
};

static const std::string vm_settings_[] = {
	"name",
	"auto-start",
	"display-name",
	"hypervisor",
	"status",
	"restore-shutdown",
	"restore-timeout",
	"restore-periodic",
	"restore-periodic-time",
	"vnc-address",
	"vnc-port",
	"qmp-socket-type",
	"qmp-address",
	"qmp-port",
	"max-attempts",
	"qemu-cmd",
	"qemu-snapshot-mode",
	"turns-enabled",
	"turn-time",
	"votes-enabled",
	"vote-time",
	"vote-cooldown-time",
	"agent-enabled",
	"agent-socket-type",
	"agent-use-virtio",
	"agent-address",
	"agent-port",
	"restore-heartbeat",
	"heartbeat-timeout",
	"uploads-enabled",
	"upload-cooldown-time",
	"upload-max-size",
	"upload-max-filename",
	"motd"
};

static const std::string hypervisor_names_[] {
	"qemu",
	"virtualbox",
	"vmware"
};

static const std::string snapshot_modes_[] {
	"off",
	"vm",
	"hd"
};

void IgnorePipe();

CollabVMServer::CollabVMServer(net::io_service& service)
	: service_(service),
	  server_(std::make_shared<CollabVMServer::Server>(service)),
	  stopping_(false),
	  process_thread_running_(false),
	  keep_alive_timer_(service),
	  vm_preview_timer_(service),
	  ip_data_timer(service),
	  ip_data_timer_running_(false),
	  guest_rng_(1000, 99999),
	  rng_(std::chrono::steady_clock::now().time_since_epoch().count()),
	  chat_history_(new ChatMessage[database_.Configuration.ChatMsgHistory]),
	  chat_history_begin_(0),
	  chat_history_end_(0),
	  chat_history_count_(0),
	  upload_count_(0) {
	// Create VMControllers for all VMs that will be auto-started
	for(auto [id, vm] : database_.VirtualMachines) {
		if(vm->AutoStart) {
			CreateVMController(vm);
		}
	}

	// set up access channels to only log interesting things
	//server_.clear_access_channels(websocketpp::log::alevel::all);
	//server_.clear_error_channels(websocketpp::log::elevel::all);

	// Initialize Asio Transport
	/*
	try
	{
		server_.init_asio(&service);
	}
	catch (const websocketpp::exception& ex)
	{
		std::cout << "Failed to initialize ASIO" << std::endl;
		throw ex;
	}
	 */
}

CollabVMServer::~CollabVMServer() {
	delete[] chat_history_;
}

std::shared_ptr<VMController> CollabVMServer::CreateVMController(const std::shared_ptr<VMSettings>& vm) {
	std::shared_ptr<VMController> controller;

	switch(vm->Hypervisor) {
		case VMSettings::HypervisorEnum::kQEMU: {
			controller = std::dynamic_pointer_cast<VMController>(std::make_shared<QEMUController>(*this, service_, vm));
		} break;
		default:
			throw std::runtime_error("Error: unsupported hypervisor in database");
	}

	vm_controllers_[vm->Name] = controller;
	return controller;
}

void CollabVMServer::Run(uint16_t port, std::string doc_root) {
	using namespace std::placeholders;

	// The path shouldn't end in a slash
	char last = doc_root[doc_root.length() - 1];
	if(last == '/' || last == '\\')
		doc_root = doc_root.substr(0, doc_root.length() - 1);
	doc_root_ = doc_root;

	server_->set_verify_handler(std::bind(&CollabVMServer::OnValidate, this, _1));
	server_->set_open_handler(std::bind(&CollabVMServer::OnOpen, this, _1));
	server_->set_close_handler(std::bind(&CollabVMServer::OnClose, this, _1));
	server_->set_message_handler(std::bind(&CollabVMServer::OnMessageFromWS, this, _1, _2));

	// Split blacklisted usernames into array
	boost::split(blacklisted_usernames_, database_.Configuration.BlacklistedNames, boost::is_any_of(";"));

	// retains compatibility with previous server behaviour
	server_->start("0.0.0.0", port);

	boost::system::error_code asio_ec;
	vm_preview_timer_.expires_from_now(std::chrono::seconds(kVMPreviewInterval), asio_ec);
	vm_preview_timer_.async_wait(std::bind(&CollabVMServer::VMPreviewTimerCallback, shared_from_this(), std::placeholders::_1));

#ifdef USE_JPEG
	// Set the JPEG quality
	if(database_.Configuration.JPEGQuality <= 100)
		SetJPEGQuality(database_.Configuration.JPEGQuality);
#endif

	// Start all of the VMs that should be auto-started
	for(auto [id, vm] : vm_controllers_) {
		vm->Start();
	}

	// Start message processing thread
	process_thread_running_ = true;
	process_thread_ = std::thread(std::bind(&CollabVMServer::ProcessingThread, shared_from_this()));

	// Once this function returns,
	// io_service::run() is called in main()
	// so we do not bother here.
}

bool CollabVMServer::FindIPv4Data(const boost::asio::ip::address_v4& addr, IPData*& ip_data) {
	auto it = ipv4_data_.find(addr.to_ulong());
	if(it == ipv4_data_.end())
		return false;
	ip_data = it->second;
	return true;
}

bool CollabVMServer::FindIPData(const boost::asio::ip::address& addr, IPData*& ip_data) {
	if(addr.is_v4()) {
		return FindIPv4Data(addr.to_v4(), ip_data);
	} else {
		const boost::asio::ip::address_v6& addr_v6 = addr.to_v6();
		if(addr_v6.is_v4_mapped()) {
			return FindIPv4Data(addr_v6.to_v4(), ip_data);
		} else {
			typedef boost::asio::detail::array<unsigned char, 16> bytes_type;
			auto it = ipv6_data_.find(addr_v6.to_bytes());
			if(it == ipv6_data_.end())
				return false;
			ip_data = it->second;
		}
	}
	return true;
}

IPData* CollabVMServer::CreateIPv4Data(const boost::asio::ip::address_v4& addr, bool one_connection) {
	unsigned long ipv4 = addr.to_ulong();
	IPData* ip_data = new IPv4Data(ipv4, one_connection);
	ipv4_data_[ipv4] = ip_data;
	return ip_data;
}

IPData* CollabVMServer::CreateIPData(const boost::asio::ip::address& addr, bool one_connection) {
	if(addr.is_v4()) {
		return CreateIPv4Data(addr.to_v4(), one_connection);
	} else {
		const boost::asio::ip::address_v6& addr_v6 = addr.to_v6();
		if(addr_v6.is_v4_mapped()) {
			return CreateIPv4Data(addr_v6.to_v4(), one_connection);
		} else {
			const std::array<unsigned char, 16>& ipv6 = addr_v6.to_bytes();
			IPData* ip_data = new IPv6Data(ipv6, one_connection);
			ipv6_data_[ipv6] = ip_data;
			return ip_data;
		}
	}
}

void CollabVMServer::DeleteIPData(IPData& ip_data) {
	if(ip_data.type == IPData::IPType::kIPv4)
		ipv4_data_.erase(static_cast<IPv4Data&>(ip_data).addr);
	else
		ipv6_data_.erase(static_cast<IPv6Data&>(ip_data).addr);

	delete &ip_data;
}

bool CollabVMServer::OnValidate(std::weak_ptr<websocketmm::websocket_user> handle) {
	if(auto handle_sp = handle.lock()) {
		auto do_subprotocol_check = [&](beast::string_view offered_tokens) -> bool {
			// tokenize the Sec-Websocket-Protocol header offered by the client
			http::token_list offered(offered_tokens);
			// TODO

			constexpr std::array<beast::string_view, 1> supported {
				"guacamole"
			};

			for(auto proto : supported) {
				auto iter = std::find(offered.begin(), offered.end(), proto);

				if(iter != offered.end()) {
					// Select compatible subprotocol
					handle_sp->SelectSubprotocol((*iter).to_string());
					return true;
				}
			}

			return false;
		};

		if(do_subprotocol_check(handle_sp->GetSubprotocols())) {
			// Create new IPData object
			const boost::asio::ip::address& addr = handle_sp->socket().remote_endpoint().address();

			std::unique_lock<std::mutex> ip_lock(ip_lock_);
			IPData* ip_data;

			if(FindIPData(addr, ip_data)) {
				if(ip_data->connections >= database_.Configuration.MaxConnections)
					return false;
				// Reuse existing IPData
				ip_data->connections++;
			} else {
				// Create new IPData
				ip_data = CreateIPData(addr, true);
			}

			ip_lock.unlock();

			handle_sp->GetUserData().user = std::make_shared<CollabVMUser>(handle, *ip_data);
			return true;
		}
	}
	return false;
}

void CollabVMServer::OnOpen(std::weak_ptr<websocketmm::websocket_user> handle) {
	if(auto handle_sp = handle.lock()) {
		PostAction<UserAction>(*handle_sp->GetUserData().user, ActionType::kAddConnection);
	}
}

void CollabVMServer::OnClose(std::weak_ptr<websocketmm::websocket_user> handle) {
	if(auto handle_sp = handle.lock()) {
		// copy the user so we can keep it around
		auto user = handle_sp->GetUserData().user;
		PostAction<UserAction>(*handle_sp->GetUserData().user, ActionType::kRemoveConnection);
	}
}

/*
std::string CollabVMServer::GenerateUuid()
{
	char buffer[UUID_LEN_STR + 1];

	uuid_t* uuid;

    // Attempt to create UUID objec
	if (uuid_create(&uuid) != UUID_RC_OK) {
		throw std::string("Could not allocate memory for UUID");
	}

    // Generate random UUID
	if (uuid_make(uuid, UUID_MAKE_V4) != UUID_RC_OK) {
		uuid_destroy(uuid);
		throw std::string("UUID generation failed");
	}

	char* identifier = buffer;
	size_t identifier_length = sizeof(buffer);

    // Build connection ID from UUID
	if (uuid_export(uuid, UUID_FMT_STR, &identifier, &identifier_length) != UUID_RC_OK) {
		uuid_destroy(uuid);
		throw std::string("Conversion of UUID to connection ID failed");
	}

	uuid_destroy(uuid);

	return std::string(buffer, identifier_length-1);
}
*/

void CollabVMServer::OnMessageFromWS(std::weak_ptr<websocketmm::websocket_user> handle, std::shared_ptr<const websocketmm::websocket_message> msg) {
	if(auto handle_sp = handle.lock()) {
		// Do not respond to binary messages.
		if(msg->message_type != websocketmm::websocket_message::type::text)
			return;

		PostAction<MessageAction>(msg, *handle_sp->GetUserData().user, ActionType::kMessage);
	}
}

void CollabVMServer::SendWSMessage(CollabVMUser& user, const std::string& str) {
	if(!server_->send_message(user.handle, websocketmm::BuildWebsocketMessage(str))) {
		if(user.connected) {
			// Disconnect the client if an error occurs
			PostAction<UserAction>(user, ActionType::kRemoveConnection);
		}
	}
}

void CollabVMServer::TimerCallback(const boost::system::error_code& ec, ActionType action) {
	if(ec)
		return;

	PostAction<Action>(action);
}

void CollabVMServer::VMPreviewTimerCallback(const boost::system::error_code ec) {
	if(ec)
		return;

	PostAction<Action>(ActionType::kUpdateThumbnails);

	boost::system::error_code asio_ec;
	vm_preview_timer_.expires_from_now(std::chrono::seconds(kVMPreviewInterval), asio_ec);
	vm_preview_timer_.async_wait(std::bind(&CollabVMServer::VMPreviewTimerCallback, shared_from_this(), std::placeholders::_1));
}

void CollabVMServer::IPDataTimerCallback(const boost::system::error_code& ec) {
	if(ec)
		return;

	std::lock_guard<std::mutex> lock(ip_lock_);
	// The IP data clean up timer should continue to run if there
	// are IPData objects without any connections
	bool should_run = false;
	IPData* ip_data;
	auto ipv4_it = ipv4_data_.begin();
	while(ipv4_it != ipv4_data_.end()) {
		ip_data = ipv4_it->second;
		if(!ip_data->connections) {
			if(ShouldCleanUpIPData(*ip_data)) {
				ipv4_it = ipv4_data_.erase(ipv4_it);
				continue;
			} else {
				should_run = true;
			}
		}
		ipv4_it++;
	}

	auto ipv6_it = ipv6_data_.begin();
	while(ipv6_it != ipv6_data_.end()) {
		ip_data = ipv6_it->second;
		if(!ip_data->connections) {
			if(ShouldCleanUpIPData(*ip_data)) {
				ipv6_it = ipv6_data_.erase(ipv6_it);
				continue;
			} else {
				should_run = true;
			}
		}
		ipv6_it++;
	}

	ip_data_timer_running_ = should_run;

	if(should_run) {
		boost::system::error_code ec;
		ip_data_timer.expires_from_now(std::chrono::minutes(kIPDataTimerInterval), ec);
		ip_data_timer.async_wait(std::bind(&CollabVMServer::IPDataTimerCallback, shared_from_this(), std::placeholders::_1));
	}
}

bool CollabVMServer::ShouldCleanUpIPData(IPData& ip_data) const {
	//if (ip_data.connections)
	//	return false;

	auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());

	if((now - ip_data.last_chat_msg).count() < database_.Configuration.ChatRateTime)
		return false;

	if(ip_data.chat_muted) {
		if((now - ip_data.last_chat_msg).count() >= database_.Configuration.ChatMuteTime && ip_data.chat_muted == kTempMute) {
			ip_data.chat_muted = kUnmuted;
		} else {
			return false;
		}
	}

	if(ip_data.name_fixed) {
		if((now - ip_data.last_name_chg).count() >= database_.Configuration.ChatMuteTime) {
			ip_data.name_fixed = false;
		} else {
			return false;
		}
	}

	if(ip_data.turn_fixed) {
		if((now - ip_data.last_turn).count() >= database_.Configuration.ChatMuteTime) {
			ip_data.turn_fixed = false;
		} else {
			return false;
		}
	}

	if(ip_data.failed_logins) {
		if((now - ip_data.failed_login_time).count() >= kLoginIPBlockTime) {
			// Reset login attempts after block time has elapsed
			ip_data.failed_logins = 0;
		} else {
			return false;
		}
	}

	for(auto&& vote : ip_data.votes)
		if(vote.second != IPData::VoteDecision::kNotVoted)
			return false;

	if(ip_data.upload_in_progress || (ip_data.next_upload_time - now).count() > 0)
		return false;

	return true;
}

//TODO(qol): Some of this should be done in destruction.

void CollabVMServer::RemoveConnection(std::shared_ptr<CollabVMUser>& user) {
	//if (!user->connected)
	//	return;

	std::cout << "[WebSocket Disconnect] IP: " << user->ip_data.GetIP();
	if(user->username)
		std::cout << " Username: \"" << *user->username << '"';
	std::cout << std::endl;

	if(user->admin_connected) {
		admin_connections_.erase(user);
		user->admin_connected = false;
	}

	// CancelFileUpload should be called before setting vm_controller
	// to a nullptr (which is done by VMController::RemoveUser)
	CancelFileUpload(*user);

	if(user->vm_controller)
		user->vm_controller->RemoveUser(user);

	delete user->guac_user;

	if(user->username) {
		// Remove the connection data from the map
		usernames_.erase(*user->username);
		// Send a remove user instruction to everyone
		std::ostringstream ss("7.remuser,1.1,", std::ostringstream::in | std::ostringstream::out | std::ostringstream::ate);
		ss << user->username->length() << '.' << *user->username << ';';
		std::string instr = ss.str();

		for(const auto& user_ : connections_) {
			//std::shared_ptr<CollabVMUser> user = *it;
			SendWSMessage(*user_, instr);
		}

		user->username.reset();
	}

	std::unique_lock<std::mutex> ip_lock(ip_lock_);

	if(!--user->ip_data.connections && ShouldCleanUpIPData(user->ip_data)) {
		DeleteIPData(user->ip_data);
	} else if(!ip_data_timer_running_) {
		// Start the IP data clean up timer if it's not already running
		ip_data_timer_running_ = true;
		boost::system::error_code ec;
		ip_data_timer.expires_from_now(std::chrono::minutes(kIPDataTimerInterval), ec);
		ip_data_timer.async_wait(std::bind(&CollabVMServer::IPDataTimerCallback, shared_from_this(), std::placeholders::_1));
	}

	ip_lock.unlock();

	user->connected = false;
}

void CollabVMServer::UpdateVMStatus(const std::string& vm_name, VMController::ControllerState state) {
	if(admin_connections_.empty())
		return;

	std::string state_str = std::to_string(static_cast<uint32_t>(state));
	std::ostringstream ss("5.admin,1.3,", std::ostringstream::in | std::ostringstream::out | std::ostringstream::ate);
	ss << vm_name.length() << '.' << vm_name << ',' << state_str.length() << '.' << state_str << ';';
	std::string instr = ss.str();

	for(const auto& admin_connection : admin_connections_) {
		assert(admin_connection->admin_connected);
		SendWSMessage(*admin_connection, instr);
	}
}

void CollabVMServer::ProcessingThread() {
	IgnorePipe();
	while(true) {
		std::unique_lock<std::mutex> lock(process_queue_lock_);
		while(process_queue_.empty()) {
			process_wait_.wait(lock);
		}

		Action* action = process_queue_.front();
		process_queue_.pop();

		lock.unlock();

		switch(action->action) {
			case ActionType::kMessage: {
				MessageAction* msg_action = static_cast<MessageAction*>(action);
				if(msg_action->user->connected) {
					if(msg_action->message) {
						GuacInstructionParser::ParseInstruction(*this, msg_action->user, std::string((char*)msg_action->message->data.data(), msg_action->message->data.size()));
					}
				}
				break;
			}
			case ActionType::kAddConnection: {
				const std::shared_ptr<CollabVMUser>& user = static_cast<UserAction*>(action)->user;
				assert(!user->connected);

				connections_.insert(user);

				// Start the keep-alive timer after the first client connects
				if(connections_.size() == 1) {
					boost::system::error_code ec;
					keep_alive_timer_.expires_from_now(std::chrono::seconds(kKeepAliveInterval), ec);
					keep_alive_timer_.async_wait(std::bind(&CollabVMServer::TimerCallback, shared_from_this(), std::placeholders::_1, ActionType::kKeepAlive));
				}

				user->connected = true;

				std::cout << "[WebSocket Connect] IP: " << user->ip_data.GetIP() << std::endl;

				break;
			}
			case ActionType::kRemoveConnection: {
				std::shared_ptr<CollabVMUser>& connection = static_cast<UserAction*>(action)->user;

				//			if(!connection) {
				//			    break;
				//			}

				if(connection->connected) {
					connections_.erase(connection);
					RemoveConnection(connection);
					connection.reset();
				}
				break;
			}
			case ActionType::kTurnChange: {
				const std::shared_ptr<VMController>& controller = static_cast<VMAction*>(action)->controller;
				controller->NextTurn();
				break;
			}
			case ActionType::kVoteEnded: {
				const std::shared_ptr<VMController>& controller = static_cast<VMAction*>(action)->controller;
				controller->EndVote();
				break;
			}
			case ActionType::kAgentConnect: {
				AgentConnectAction* agent_action = static_cast<AgentConnectAction*>(action);
				const std::shared_ptr<VMController>& controller = agent_action->controller;
				controller->agent_connected_ = true;
				controller->agent_os_name_ = agent_action->os_name;
				controller->agent_service_pack_ = agent_action->service_pack;
				controller->agent_pc_name_ = agent_action->pc_name;
				controller->agent_username_ = agent_action->username;
				controller->agent_max_filename_ = std::min(static_cast<uint32_t>(controller->GetSettings().UploadMaxFilename),
														   agent_action->max_filename);
				controller->agent_upload_in_progress_ = false;

				std::cout << "Agent Connected, OS: \"" << agent_action->os_name << "\", SP: \"" << agent_action->service_pack << "\", PC: \"" << agent_action->pc_name << "\", Username: \"" << agent_action->username << "\"" << std::endl;

				SendActionInstructions(*controller, controller->GetSettings());
				break;
			}
			case ActionType::kAgentDisconnect: {
				const std::shared_ptr<VMController>& controller = static_cast<VMAction*>(action)->controller;
				controller->agent_connected_ = false;
				controller->agent_os_name_.clear();
				controller->agent_service_pack_.clear();
				controller->agent_pc_name_.clear();
				controller->agent_username_.clear();
				controller->agent_upload_in_progress_ = false;

				SendActionInstructions(*controller, controller->GetSettings());
				break;
			}
			/*
		case ActionType::kHttpUploadTimedout:
		{
			const std::shared_ptr<UploadInfo>& upload_info = static_cast<HttpAction*>(action)->upload_info;
			if (upload_info->canceled)
				break;
			upload_info->canceled = true;

			unique_lock<std::mutex> lock(upload_lock_);
			if (upload_info->upload_it != upload_ids_.end())
			    upload_ids_.erase(upload_info->upload_it);
			lock.unlock();
			if (auto user = upload_info->user.lock())
			{
				user->upload_info.reset();
				FileUploadEnded(upload_info, &user, FileUploadResult::kHttpUploadTimedOut);
			}
			else
			{
				FileUploadEnded(upload_info, nullptr, FileUploadResult::kHttpUploadTimedOut);
			}
			break;
		}
		case ActionType::kHttpUploadFinished:
		{
			const std::shared_ptr<UploadInfo>& upload_info = static_cast<HttpAction*>(action)->upload_info;
			if (upload_info->canceled)
				break;
			upload_info->canceled = true;

			if (boost::asio::steady_timer* timer = upload_info->timeout_timer)
			{
				upload_info->timeout_timer = nullptr;
				boost::system::error_code ec;
				timer->cancel(ec);
				delete timer;
			}

			VMController& vm_controller = *upload_info->vm_controller;
			if (auto user = upload_info->user.lock())
			{
				user->upload_info.reset();

				// Cancel the upload if the user switched to another VM
				if (user->vm_controller == &vm_controller)
				{
					if (vm_controller.agent_upload_in_progress_)
					{
						vm_controller.agent_upload_queue_.push_back(upload_info);
					}
					else
					{
						vm_controller.UploadFile(upload_info);
						vm_controller.agent_upload_in_progress_ = true;
					}
					break;
				}
				FileUploadEnded(upload_info, &user, FileUploadResult::kHttpUploadFailed);
				break;
			}
			FileUploadEnded(upload_info, nullptr, FileUploadResult::kHttpUploadFailed);
			break;
		}
		case ActionType::kHttpUploadFailed:
		{
			const std::shared_ptr<UploadInfo>& upload_info = static_cast<HttpAction*>(action)->upload_info;
			if (upload_info->canceled)
				break;
			upload_info->canceled = true;

			if (auto user = upload_info->user.lock())
			{
				user->upload_info.reset();
				FileUploadEnded(upload_info, &user, FileUploadResult::kHttpUploadFailed);
			}
			else
			{
				FileUploadEnded(upload_info, nullptr, FileUploadResult::kHttpUploadFailed);
			}
			break;
		}
		case ActionType::kUploadEnded:
		{
			FileUploadAction& upload_action = *static_cast<FileUploadAction*>(action);
			FileUploadResult result = upload_action.upload_result == FileUploadAction::UploadResult::kSuccess ||
										upload_action.upload_result == FileUploadAction::UploadResult::kSuccessNoExec ?
										FileUploadResult::kAgentUploadSucceeded : FileUploadResult::kAgentUploadFailed;
			if (auto user = upload_action.upload_info->user.lock())
			{
				user->upload_info.reset();
				FileUploadEnded(upload_action.upload_info, &user, result);
			}
			else
			{
				FileUploadEnded(upload_action.upload_info, nullptr, result);
			}
			break;
		}
		 */
			case ActionType::kKeepAlive:
				if(!connections_.empty()) {
					// Disconnect all clients that haven't responded within the timeout period
					// and broadcast a nop instruction to all clients
					using std::chrono::steady_clock;
					using std::chrono::seconds;
					using std::chrono::time_point;
					time_point<steady_clock, seconds> now(std::chrono::time_point_cast<seconds>(steady_clock::now()));
					for(auto it = connections_.begin(); it != connections_.end();) {
						std::shared_ptr<CollabVMUser> user = *it;

						if((now - user->last_nop_instr).count() > kKeepAliveTimeout) {
							// Disconnect the websocket client
							if(!user->handle.expired())
								user->handle.lock()->close();

							it = connections_.erase(it);
							if(user->connected)
								RemoveConnection(user);
						} else {
							SendWSMessage(*user, "3.nop;");
							it++;
						}
					}
					// Schedule another keep-alive instruction
					if(!connections_.empty()) {
						boost::system::error_code ec;
						keep_alive_timer_.expires_from_now(std::chrono::seconds(kKeepAliveInterval), ec);
						keep_alive_timer_.async_wait(std::bind(&CollabVMServer::TimerCallback, shared_from_this(), std::placeholders::_1, ActionType::kKeepAlive));
					}
				}
				break;
			case ActionType::kUpdateThumbnails:
				for(auto& vm_controller : vm_controllers_) {
					vm_controller.second->UpdateThumbnail();
				}
				break;
			case ActionType::kVMThumbnail: {
				VMThumbnailUpdate* thumbnail = static_cast<VMThumbnailUpdate*>(action);
				thumbnail->controller->SetThumbnail(thumbnail->thumbnail);
				break;
			}
			case ActionType::kVMStateChange: {
				VMStateChange* state_change = static_cast<VMStateChange*>(action);
				std::shared_ptr<VMController>& controller = state_change->controller;

				if(state_change->state == VMController::ControllerState::kStopped) {
					VMController::StopReason reason = controller->GetStopReason();

					if(!stopping_ && reason == VMController::StopReason::kRestart) {
						controller->Start();
					} else {
						if(reason == VMController::StopReason::kError) {
							std::cout << "VM \"" << controller->GetSettings().Name << "\" was stopped. " << controller->GetErrorMessage() << std::endl;
						}

						UpdateVMStatus(controller->GetSettings().Name, VMController::ControllerState::kStopped);

						auto vm_it = vm_controllers_.find(controller->GetSettings().Name);
						if(vm_it != vm_controllers_.end())
							vm_controllers_.erase(vm_it);
						controller.reset();

						if(stopping_ && vm_controllers_.empty()) {
							// Free the io_service's work so it can stop
							//asio_work_.reset();
							delete action;
							// Exit the processing loop
							goto stop;
						}
					}
				} else {
					if(state_change->state == VMController::ControllerState::kStopping)
						controller->ClearTurnQueue();

					UpdateVMStatus(controller->GetSettings().Name, static_cast<VMStateChange*>(action)->state);
				}
				break;
			}
			case ActionType::kVMCleanUp: {
				const std::shared_ptr<VMController>& controller = static_cast<VMAction*>(action)->controller;
				controller->CleanUp();
				break;
			}
			case ActionType::kShutdown:

				// Disconnect all active clients
				for(const auto& connection : connections_) {
					if(!connection->handle.expired())
						connection->handle.lock()->close();
				}

				// If there are no VM controllers running, exit the processing thread
				if(vm_controllers_.empty()) {
					//asio_work_.reset();
					delete action;
					// Exit the processing loop
					goto stop;
				}
				std::cout << "Stopping all VM Controllers..." << std::endl;

				// Stop all VM controllers
				for(auto [id, vm] : vm_controllers_) {
					if(vm->IsRunning()) {
						vm->Stop(VMController::StopReason::kRemove);
						//vm->~VMController();
					}
				}

				// (we don't need to worry about erasing them, the CollabVMServer destructor will do that for us.)
		}

		delete action;
	}
stop:
	process_thread_running_ = false;
}

void CollabVMServer::OnVMControllerVoteEnded(const std::shared_ptr<VMController>& controller) {
	PostAction<VMAction>(controller, ActionType::kVoteEnded);
}

void CollabVMServer::OnVMControllerTurnChange(const std::shared_ptr<VMController>& controller) {
	PostAction<VMAction>(controller, ActionType::kTurnChange);
}

void CollabVMServer::OnVMControllerThumbnailUpdate(const std::shared_ptr<VMController>& controller, std::string* str) {
	PostAction<VMThumbnailUpdate>(controller, str);
}

void CollabVMServer::BroadcastTurnInfo(VMController& controller, UserList& users, const std::deque<std::shared_ptr<CollabVMUser>>& turn_queue, CollabVMUser* current_turn, uint32_t time_remaining) {
	if(current_turn == nullptr) {
		// The instruction is static if there is nobody controlling the VM
		// and nobody is waiting in the queue
		users.ForEachUser([this](CollabVMUser& user) {
			SendWSMessage(user, "4.turn,1.0,1.0;");
		});
	} else {
		std::string users_list;
		users_list.reserve((turn_queue.size() + 1) * (kMaxUsernameLen + 4));

		// Append the number users waiting in the queue plus one
		// for the user who's controlling it
		std::string temp_str = std::to_string(turn_queue.size() + 1);
		users_list += std::to_string(temp_str.length());
		users_list += '.';
		users_list += temp_str;
		users_list += ',';

		// Append name of the user currently in control
		users_list += std::to_string(current_turn->username->length());
		users_list += '.';
		users_list += *current_turn->username;

		// Append the names of the users waiting in the queue
		for(const auto& user : turn_queue) {
			users_list += ',';

			users_list += std::to_string(user->username->length());
			users_list += '.';
			users_list += *user->username;
		}

		users_list += ';';

		std::string turn_instr = "4.turn,";

		// Append time remaining for the current user's turn
		temp_str = std::to_string(time_remaining);
		turn_instr += std::to_string(temp_str.length());
		turn_instr += '.';
		turn_instr += temp_str;
		turn_instr += ',';

		turn_instr += users_list;

		// Send the instruction to the user that has control first
		current_turn->waiting_turn = false;
		SendWSMessage(*current_turn, turn_instr);

		if(!turn_queue.empty()) {
			// Replace the semicolon at the end of the string with a comma
			turn_instr.replace(turn_instr.length() - 1, 1, 1, ',');

			// The length of the turn instruction before appending the time to it
			const size_t instr_len_before = turn_instr.length();
			// Send the instruction to each user waiting in the queue and include
			// the amount of time until their turn
			const uint32_t turn_time = controller.GetSettings().TurnTime * 1000;
			uint32_t user_wait_time = time_remaining;
			for(auto it = turn_queue.begin(); it != turn_queue.end(); it++, user_wait_time += turn_time) {
				std::shared_ptr<CollabVMUser> user = *it;
				temp_str = std::to_string(user_wait_time);
				turn_instr += std::to_string(temp_str.length());
				turn_instr += '.';
				turn_instr += temp_str;
				turn_instr += ';';
				SendWSMessage(*user, turn_instr);

				// Remove the part that we just appended
				turn_instr.resize(instr_len_before);
			}

			// Replace the comma at the end of the string with a semicolon
			turn_instr.replace(turn_instr.length() - 1, 1, 1, ';');
		}

		// Tell all the spectators how many users are in the waiting queue
		users.ForEachUser([&](CollabVMUser& user) {
			if(user.waiting_turn || &user == current_turn)
				return;
			SendWSMessage(user, turn_instr);
		});
	}
}

void CollabVMServer::SendTurnInfo(CollabVMUser& user, uint32_t time_remaining, const std::string& current_turn, const std::deque<std::shared_ptr<CollabVMUser>>& turn_queue) {
	std::string instr = "4.turn,";

	// Remaining time for the current user's turn
	std::string temp_str = std::to_string(time_remaining);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ',';
	//instr += user_list;
	// Number of users
	temp_str = std::to_string(turn_queue.size() + 1);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ',';
	// Current user controlling the VM
	instr += std::to_string(current_turn.length());
	instr += '.';
	instr += current_turn;
	// Users waiting in the queue
	for(const auto& user : turn_queue) {
		instr += ',';

		instr += std::to_string(user->username->length());
		instr += '.';
		instr += *user->username;
	}

	instr += ';';

	SendWSMessage(user, instr);
}

void CollabVMServer::BroadcastVoteInfo(const VMController& vm, UserList& users, bool vote_started, uint32_t time_remaining, uint32_t yes_votes, uint32_t no_votes) {
	std::string instr = "4.vote,";
	// Opcode
	instr += vote_started ? "1.0," : "1.1,";
	// Time remaining to vote
	std::string temp_str = std::to_string(time_remaining);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ',';
	// Number of yes votes
	temp_str = std::to_string(yes_votes);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ',';
	// Number of no votes
	temp_str = std::to_string(no_votes);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;

	instr += ';';
	users.ForEachUser([&](CollabVMUser& user) {
		SendWSMessage(user, instr);
	});
}

void CollabVMServer::SendVoteInfo(const VMController& vm, CollabVMUser& user, uint32_t time_remaining, uint32_t yes_votes, uint32_t no_votes) {
	std::string instr = "4.vote,1.1,";
	// Time remaining to vote
	std::string temp_str = std::to_string(time_remaining);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ',';
	// Number of yes votes
	temp_str = std::to_string(yes_votes);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ',';
	// Number of no votes
	temp_str = std::to_string(no_votes);
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ';';

	SendWSMessage(user, instr);
}

void CollabVMServer::UserStartedVote(const VMController& vm, UserList& users, CollabVMUser& user) {
	user.ip_data.votes[&vm] = IPData::VoteDecision::kYes;

#define MSG " started a vote to reset the VM."
	std::string instr = "4.chat,0.,";
	instr += std::to_string(user.username->length() + STR_LEN(MSG));
	instr += '.';
	instr += *user.username;
	instr += MSG ";";
	user.voted_amount++;
	users.ForEachUser([&](CollabVMUser& user) {
		SendWSMessage(user, instr);
	});
}

void CollabVMServer::UserVoted(const VMController& vm, UserList& users, CollabVMUser& user, bool vote) {
	if(user.voted_limit) {
		// if you're a votebomber you shouldn't have a decision anyways
		user.ip_data.votes[&vm] = IPData::VoteDecision::kNotVoted;
		return;
	} else {
		user.ip_data.votes[&vm] = vote ? IPData::VoteDecision::kYes : IPData::VoteDecision::kNo;
	}

#define MSG_YES " voted yes."
#define MSG_NO " voted no."
	std::string instr = "4.chat,0.,";
	size_t msg_len = vote ? STR_LEN(MSG_YES) : STR_LEN(MSG_NO);
	instr += std::to_string(user.username->length() + msg_len);
	instr += '.';
	instr += *user.username;
	instr += vote ? MSG_YES ";" : MSG_NO ";";

	users.ForEachUser([&](CollabVMUser& user) {
		SendWSMessage(user, instr);
	});
}

void CollabVMServer::BroadcastVoteEnded(const VMController& vm, UserList& users, bool vote_succeeded) {
	std::string instr = vote_succeeded ? "4.chat,0.,33.The vote to reset the VM has won.;" : "4.chat,0.,34.The vote to reset the VM has lost.;";

	users.ForEachUser([&](CollabVMUser& user) {
		SendWSMessage(user, "4.vote,1.2;");
		SendWSMessage(user, instr);
		// Reset the vote amount for all users.
		// TODO: Make this only act on users who have voted at least once
		if(user.voted_amount) {
			user.voted_amount = 0;
		}
		user.voted_limit = false;
	});

	// Reset for next vote and allow IPData to be deleted
	for(const auto& ip_data : ipv4_data_)
		ip_data.second->votes[&vm] = IPData::VoteDecision::kNotVoted;
	for(const auto& ip_data : ipv6_data_)
		ip_data.second->votes[&vm] = IPData::VoteDecision::kNotVoted;
}

void CollabVMServer::VoteCoolingDown(CollabVMUser& user, uint32_t time_remaining) {
	std::string temp_str = std::to_string(time_remaining);
	std::string instr = "4.vote,1.3,";
	instr += std::to_string(temp_str.length());
	instr += '.';
	instr += temp_str;
	instr += ';';
	SendWSMessage(user, instr);
}

void CollabVMServer::Stop() {
	if(stopping_)
		return;
	stopping_ = true;

	// Don't log opertaion aborted error caused by stop_listening
	//server_.set_error_channels(websocketpp::log::elevel::none);

	server_->stop();

	//if (ws_ec)
	//	std::cout << "stop_listening error: " << ws_ec.message() << std::endl;

	// Stop the timers
	boost::system::error_code asio_ec;
	keep_alive_timer_.cancel(asio_ec);
	ip_data_timer.cancel(asio_ec);
	vm_preview_timer_.cancel(asio_ec);

	if(process_thread_running_) {
		std::unique_lock<std::mutex> lock(process_queue_lock_);
		// Delete all actions currently in the queue and add the
		// shutdown action to signal the processing queue to disconnect
		// all websocket clients and stop all VM controllers
		while(!process_queue_.empty()) {
			delete process_queue_.front();
			process_queue_.pop();
		}

		lock.unlock();

		// Clear processing queue by swapping it with an empty one
		std::queue<Action*> emptyQueue;
		process_queue_.swap(emptyQueue);

		// Add the stop action to the processing queue
		PostAction<Action>(ActionType::kShutdown);
	}

	// Wait for the processing thread to stop
	process_thread_.join();
}

void CollabVMServer::OnVMControllerStateChange(const std::shared_ptr<VMController>& controller, VMController::ControllerState state) {
	std::cout << "VM controller ID: " << controller->GetSettings().Name;
	switch(state) {
		case VMController::ControllerState::kStopped:
			std::cout << " is stopped";
			break;
		case VMController::ControllerState::kStarting:
			std::cout << " is starting";
			break;
		case VMController::ControllerState::kRunning:
			std::cout << " has been started";
			break;
		case VMController::ControllerState::kStopping:
			std::cout << " is stopping";
			break;
	}
	std::cout << std::endl;

	PostAction<VMStateChange>(controller, state);
}

void CollabVMServer::OnVMControllerCleanUp(const std::shared_ptr<VMController>& controller) {
	PostAction<VMAction>(controller, ActionType::kVMCleanUp);
}

void CollabVMServer::SendGuacMessage(std::weak_ptr<websocketmm::websocket_user> handle, const std::string& str) {
	if(!server_->send_message(handle, websocketmm::BuildWebsocketMessage(str))) {
		if(auto handle_sp = handle.lock()) {
			auto user = handle_sp->GetUserData().user;

			// Disconnect the client if an error occurs
			PostAction<UserAction>(*user, ActionType::kRemoveConnection);
		}
	}
}

void CollabVMServer::AppendChatMessage(std::ostringstream& ss, ChatMessage* chat_msg) {
	ss << ',' << chat_msg->username->length() << '.' << *chat_msg->username << ',' << chat_msg->message.length() << '.' << chat_msg->message;
}

void CollabVMServer::SendChatHistory(CollabVMUser& user) {
	if(chat_history_count_) {
		std::ostringstream ss("4.chat", std::ios_base::in | std::ios_base::out | std::ios_base::ate);
		unsigned char len = database_.Configuration.ChatMsgHistory;

		// Iterate through each of the messages in the circular buffer
		if(chat_history_end_ > chat_history_begin_) {
			for(unsigned char i = chat_history_begin_; i < chat_history_end_; i++)
				AppendChatMessage(ss, &chat_history_[i]);
		} else {
			for(unsigned char i = chat_history_begin_; i < len; i++)
				AppendChatMessage(ss, &chat_history_[i]);

			for(unsigned char i = 0; i < chat_history_end_; i++)
				AppendChatMessage(ss, &chat_history_[i]);
		}
		ss << ';';
		SendWSMessage(user, ss.str());
	}
}

bool CollabVMServer::ValidateUsername(const std::string& username) {
	if(username.length() < kMinUsernameLen || username.length() > kMaxUsernameLen)
		return false;

	// Cannot start with a space
	char c = username[0];
	if(!((c >= 'A' && c <= 'Z') ||									// Uppercase letters
		 (c >= 'a' && c <= 'z') ||									// Lowercase letters
		 (c >= '0' && c <= '9') ||									// Numbers
		 c == '_' || c == '-' || c == '.' || c == '?' || c == '!')) // Underscores, dashes, dots, question marks, exclamation points
		return false;

	bool prev_space = false;
	for(size_t i = 1; i < username.length() - 1; i++) {
		c = username[i];
		// Only allow the following characters
		if((c >= 'A' && c <= 'Z') ||								 // Uppercase letters
		   (c >= 'a' && c <= 'z') ||								 // Lowercase letters
		   (c >= '0' && c <= '9') ||								 // Numbers
		   c == '_' || c == '-' || c == '.' || c == '?' || c == '!') // Spaces, underscores, dashes, dots, question marks, exclamation points
		{
			prev_space = false;
			continue;
		}
		// Check that the previous character was not a space
		if(!prev_space && c == ' ') {
			prev_space = true;
			continue;
		}
		return false;
	}

	// Cannot end with a space
	c = username[username.length() - 1];
	if((c >= 'A' && c <= 'Z') ||								 // Uppercase letters
	   (c >= 'a' && c <= 'z') ||								 // Lowercase letters
	   (c >= '0' && c <= '9') ||								 // Numbers
	   c == '_' || c == '-' || c == '.' || c == '?' || c == '!') // Underscores, dashes, dots, question marks, exclamation points
		return true;

	return false;
}

void CollabVMServer::SendOnlineUsersList(CollabVMUser& user) {
	std::ostringstream ss("7.adduser,", std::ios_base::in | std::ios_base::out | std::ios_base::ate);
	std::string num = std::to_string(usernames_.size());
	ss << num.size() << '.' << num;

	for(auto& username : usernames_) {
		std::shared_ptr<CollabVMUser> data = username.second;
		// Append the user to the online users list
		num = std::to_string(data->user_rank);
		ss << ',' << data->username->length() << '.' << *data->username << ',' << num.size() << '.' << num;
	}
	ss << ';';
	SendWSMessage(user, ss.str());
}

void CollabVMServer::ChangeUsername(const std::shared_ptr<CollabVMUser>& data, const std::string& new_username, UsernameChangeResult result, bool send_history) {
	// Ratelimit these username changes
	auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());
	if(data->ip_data.name_fixed) {
		if((now - data->ip_data.last_name_chg).count() >= database_.Configuration.NameMuteTime)
			data->ip_data.name_fixed = false;
		else
			return;
	}
	if(database_.Configuration.NameRateCount && database_.Configuration.NameRateTime) {
		// Calculate the time since the user's last name change
		if((now - data->ip_data.last_name_chg).count() < database_.Configuration.NameRateTime) {
			if(++data->ip_data.name_chg_count >= database_.Configuration.NameRateCount) {
				std::string mute_time = std::to_string(database_.Configuration.NameMuteTime);
				std::cout << "[Anti-Namefag] User prevented from changing usernames. It has been stopped for " << mute_time << " seconds. IP: " << data->ip_data.GetIP() << std::endl;
				// Keep the user from changing their name for attempting to go over the
				// name change limit
				data->ip_data.last_name_chg = now;
				data->ip_data.name_fixed = true;
				return;
			}
		} else {
			data->ip_data.name_chg_count = 0;
		}
	}

	if(data->ip_data.name_fixed) {
		return;
	}

	// Send a rename instruction to the client telling them their username
	std::string instr;
	instr = "6.rename,1.0,1.";
	instr += (char)result;
	instr += ',';
	instr += std::to_string(new_username.length());
	instr += '.';
	instr += new_username;
	instr += ',';
	std::string rank = std::to_string(data->user_rank);
	instr += std::to_string(rank.length());
	instr += '.';
	instr += rank;
	instr += ';';
	SendWSMessage(*data, instr);

	// If the result of the username change was not successful
	// then don't broadcast the change to any other clients
	if(result != UsernameChangeResult::kSuccess)
		return;

	if(!usernames_.empty()) {
		// If the client did not previously have a username then
		// it means the client is joining the server
		bool user_joining = !data->username;

		// Create an adduser instruction if the client is joining,
		// otherwise create a rename instruction to update the user's username
		if(user_joining) {
			instr = "7.adduser,1.1,";
		} else {
			instr = "6.rename,1.1,";
			// Append the old username
			instr += std::to_string(data->username->length());
			instr += '.';
			instr += *data->username;
			instr += ',';
		}
		// Append the new username and user type to the instruction
		instr += std::to_string(new_username.length());
		instr += '.';
		instr += new_username;
		instr += ',';
		instr += std::to_string(rank.length());
		instr += '.';
		instr += rank;
		instr += ';';

		// Send instruction to all users viewing a VM
		for(const auto& user : connections_) {
			if(user->vm_controller)
				SendWSMessage(*user, instr);
		}
	}

	// If the user had an old username delete it from the usernames_ map
	if(data->username) {
		std::cout << "[Username Changed] IP: " << data->ip_data.GetIP() << " Old: \"" << *data->username << "\" New: \"" << new_username << '"' << std::endl;
		usernames_.erase(*data->username);
		data->username->assign(new_username);
	} else {
		data->username = std::make_shared<std::string>(new_username);
		std::cout << "[Username Assigned] IP: " << data->ip_data.GetIP() << " New username: \"" << new_username << '"' << std::endl;
	}

	data->ip_data.name_chg_count++;
	data->ip_data.last_name_chg = now;
	usernames_[new_username] = data;
}

std::string CollabVMServer::GenerateUsername() {
	// If the username is already taken generate a new one
	uint32_t num = guest_rng_(rng_);

	constexpr static auto username_base = "guest";

	std::string username = username_base + std::to_string(num);

	// Increment the number until a username is found that is not taken
	while(usernames_.find(username) != usernames_.end()) {
		username = username_base + std::to_string(++num);
	}

	return username;
}

std::string CollabVMServer::EncodeHTMLString(const char* str, size_t strLen) {
	std::ostringstream ss;
	for(size_t i = 0; i < strLen; i++) {
		char c = str[i];
		switch(c) {
			case '&':
				ss << "&amp;";
				break;
			case '<':
				ss << "&lt;";
				break;
			case '>':
				ss << "&gt;";
				break;
			case '"':
				ss << "&quot;";
				break;
			case '\'':
				ss << "&#x27;";
				break;
			case '/':
				ss << "&#x2F;";
				break;
			case '\n':
				ss << "&#13;&#10;";
				break;
			default:
				// Only allow printable ASCII characters
				if(c >= 32 && c <= 126)
					ss << c;
		}
	}
	return ss.str();
}

void CollabVMServer::ExecuteCommandAsync(std::string command) {
	// system() is used for simplicity but it is actually synchronous,
	// so it is called in a new thread
	std::string command_ = command;
	std::thread([command_] {
		if(std::system(command_.c_str())) {
			std::cout << "An error occurred while executing: " << command_ << std::endl;
		};
	})
	.detach();
}

void CollabVMServer::MuteUser(const std::shared_ptr<CollabVMUser>& user, bool permanent) {
	auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());

	std::string mute_time = std::to_string(database_.Configuration.ChatMuteTime);
	std::cout << "[Chat] User was muted ";
	if(permanent)
		std::cout << "indefinitely.";
	else
		std::cout << "for " << mute_time << " seconds.";
	std::cout << " IP: " << user->ip_data.GetIP() << " Username: \"" << *user->username << '"' << std::endl;
	// Mute the user
	user->ip_data.last_chat_msg = now;
	user->ip_data.chat_muted = permanent ? kPermMute : kTempMute;

#define part1 "You have been muted"
#define part2 " for "
#define part3 " seconds."
	std::string instr = "4.chat,0.,";
	if(permanent)
		instr += std::to_string(sizeof(part1));
	else
		instr += std::to_string(mute_time.length() + sizeof(part1) + sizeof(part2) + sizeof(part3) - 3);
	instr += "." part1;
	if(permanent)
		instr += ".";
	else {
		instr += part2;
		instr += mute_time;
		instr += part3;
	}
	instr += ";";
	SendWSMessage(*user, instr);
	//return;
}

void CollabVMServer::UnmuteUser(const std::shared_ptr<CollabVMUser>& user) {
	user->ip_data.chat_muted = kUnmuted;
	std::cout << "[Chat] User was unmuted."
				 " IP: "
			  << user->ip_data.GetIP() << " Username: \"" << *user->username << '"' << std::endl;
#define part1u "You have been unmuted."
	std::string instr = "4.chat,0.,";
	instr += std::to_string(sizeof(part1u) - 1);
	instr += "." part1u;
	instr += ";";
	SendWSMessage(*user, instr);
	//return;
}

void CollabVMServer::OnMouseInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args) {
	// Only allow a user to send mouse instructions if it is their turn,
	// they are an admin, or they are connected to the admin panel
	if(user->vm_controller != nullptr &&
	   (user->vm_controller->CurrentTurn() == user ||
		user->user_rank == UserRank::kAdmin ||
		user->admin_connected) &&
	   user->guac_user != nullptr && user->guac_user->client_) {
		user->guac_user->client_->HandleMouse(*user->guac_user, args);
	}
}

void CollabVMServer::OnKeyInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args) {
	// Only allow a user to send keyboard instructions if it is their turn,
	// they are an admin, or they are connected to the admin panel
	if(user->vm_controller != nullptr &&
	   (user->vm_controller->CurrentTurn() == user ||
		user->user_rank == UserRank::kAdmin ||
		user->admin_connected) &&
	   user->guac_user != nullptr && user->guac_user->client_) {
		user->guac_user->client_->HandleKey(*user->guac_user, args);
	}
}

void CollabVMServer::OnRenameInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args) {
	if(args.empty()) {
		// The users wants the server to generate a username for them
		if(!user->username) {
			ChangeUsername(user, GenerateUsername(), UsernameChangeResult::kSuccess, false);
		}
		return;
	}
	// The requested username
	std::string username = args[0];

	// Check if the requested username and current usernames are the same
	if(user->username && *user->username == username)
		return;

	// Whether a new username should be generated for the user
	bool gen_username = false;
	UsernameChangeResult result;

	if(username.empty()) {
		gen_username = true;
		result = UsernameChangeResult::kSuccess;
	} else if(!ValidateUsername(username)) {
		// The requested username is invalid
		if(!user->username) {
			// Respond with successful result and generate a new
			// username so the user can join
			gen_username = true;
			result = UsernameChangeResult::kSuccess;
		} else {
			result = UsernameChangeResult::kInvalid;
		}
	} else if(usernames_.find(username) != usernames_.end()) {
		// The requested username is already taken
		if(!user->username) {
			// Respond with successful result and generate a new
			// username so the user can join
			gen_username = true;
			result = UsernameChangeResult::kSuccess;
		} else {
			result = UsernameChangeResult::kUsernameTaken;
		}
	} else {
		if(!gen_username) {
			if(std::find(blacklisted_usernames_.begin(), blacklisted_usernames_.end(), username) != blacklisted_usernames_.end() && user->user_rank != UserRank::kModerator && user->user_rank != UserRank::kAdmin) {
				// The requested username is blacklisted
				result = UsernameChangeResult::kBlacklisted;
				std::string instr;
				instr = "6.rename,1.0,1.3,";
				if(!user->username) {
					instr += "0.,";
				} else {
					instr += std::to_string(user->username->length());
					instr += '.';
					instr += *user->username;
					instr += ',';
				}
				std::string rank = std::to_string(user->user_rank);
				instr += std::to_string(rank.length());
				instr += '.';
				instr += rank;
				instr += ';';
				SendWSMessage(*user, instr);
				return;
			}
		}

		// The requested username is valid and available
		ChangeUsername(user, username, UsernameChangeResult::kSuccess, args.size() > 1);
		return;
	}

	ChangeUsername(user, gen_username ? GenerateUsername() : *user->username, result, args.size() > 1);
}

/**
 * Appends the VM actions to the instruction.
 */
static void AppendVMActions(const VMController& controller, const VMSettings& settings, std::string& instr) {
	instr += settings.TurnsEnabled ? "1,1." : "0,1.";
	instr += settings.VotesEnabled ? "1,1." : "0,1.";
	if(settings.UploadsEnabled && controller.agent_connected_) {
		instr += "1,";
		std::string temp = std::to_string(settings.MaxUploadSize);
		instr += std::to_string(temp.length());
		instr += '.';
		instr += temp;
		instr += ',';
		temp = std::to_string(controller.agent_max_filename_);
		instr += std::to_string(temp.length());
		instr += '.';
		instr += temp;
		instr += ';';
	} else {
		instr += "0;";
	}
}

void CollabVMServer::SendActionInstructions(VMController& controller, const VMSettings& settings) {
	std::string instr = "6.action,1.";
	AppendVMActions(controller, settings, instr);
	controller.GetUsersList().ForEachUser([this, &instr](CollabVMUser& user) {
		SendWSMessage(user, instr);
	});
}

void CollabVMServer::BroadcastMOTD(VMController& controller, const VMSettings& settings) {
	std::string instr = "4.chat,0.,";
	instr += std::to_string(settings.MOTD.length());
	instr += '.';
	instr += settings.MOTD;
	instr += ';';

	controller.GetUsersList().ForEachUser([self = shared_from_this(), &instr](CollabVMUser& user) {
		self->SendWSMessage(user, instr);
	});
}

void CollabVMServer::OnConnectInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args) {
	if(args.size() != 1 || user->guac_user != nullptr || !user->username) {
		return;
	}

	// Connect
	std::string vm_name = args[0];
	if(vm_name.empty()) {
		// Invalid VM ID
		SendWSMessage(*user, "7.connect,1.0;");
		return;
	}

	auto it = vm_controllers_.find(vm_name);
	if(it == vm_controllers_.end()) {
		// VM not found
		SendWSMessage(*user, "7.connect,1.0;");
		return;
	}

	VMController& controller = *it->second;

	// Send cooldown time before action instruction
	if(user->ip_data.upload_in_progress)
		SendWSMessage(*user, "4.file,1.6;");
	else
		SendUploadCooldownTime(*user, controller);

	if(user->ip_data.name_fixed)
		return;

	/*if (user->ip_data.turn_fixed)
		return;*/

	std::string instr = "7.connect,1.1,1.";
	AppendVMActions(controller, controller.GetSettings(), instr);
	SendWSMessage(*user, instr);

	SendOnlineUsersList(*user);
	SendChatHistory(*user);

	// Send the MOTD if it's not empty.
	if(!controller.GetSettings().MOTD.empty()) {
		instr = "4.chat,0.,";
		instr += std::to_string(controller.GetSettings().MOTD.length());
		instr += '.';
		instr += controller.GetSettings().MOTD;
		instr += ';';
		SendWSMessage(*user, instr);
	}

	user->guac_user = new GuacUser(this, user->handle);
	controller.AddUser(user);
}

void CollabVMServer::OnAdminInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args) {
	// This instruction should have at least one argument
	if(args.empty())
		return;

	std::string str = args[0];
	if(str.length() > 2)
		return;

	int opcode;
	try {
		opcode = std::stoi(str);
	} catch(...) {
		return;
	}

	if(!user->admin_connected && opcode != kSeshID && opcode != kMasterPwd && (user->user_rank != UserRank::kModerator || !database_.Configuration.ModEnabled))
		return;

	if(user->user_rank == UserRank::kModerator && opcode != kStop && opcode != kSeshID && opcode != kMasterPwd && (opcode != kRestoreVM || !(database_.Configuration.ModPerms & 1)) && (opcode != kResetVM || !(database_.Configuration.ModPerms & 2)) && (opcode != kBanUser || !(database_.Configuration.ModPerms & 4)) && (opcode != kForceVote || !(database_.Configuration.ModPerms & 8)) && (opcode != kMuteUser || !(database_.Configuration.ModPerms & 16)) && (opcode != kKickUser || !(database_.Configuration.ModPerms & 32)) && (opcode != kEndUserTurn || !(database_.Configuration.ModPerms & 64)) && (opcode != kClearTurnQueue || !(database_.Configuration.ModPerms & 64)) && (opcode != kForceTakeTurn || !(database_.Configuration.ModPerms & 64)) && (opcode != kRenameUser || !(database_.Configuration.ModPerms & 128)) && (opcode != kUserIP || !(database_.Configuration.ModPerms & 256)))
		return;

	switch(opcode) {
		case kStop:
			// Logged out
			SendWSMessage(*user, "5.admin,1.0,1.4;");
			user->user_rank = UserRank::kUnregistered;
			if(user->vm_controller != nullptr) {
				// Send new rank to users
				std::string adminUser = "7.adduser,1.1,";
				std::string adminStr = *user->username;
				adminUser += std::to_string(adminStr.length());
				adminUser += ".";
				adminUser += adminStr;
				adminUser += ",1.0;";
				user->vm_controller->GetUsersList().ForEachUser([&](CollabVMUser& data) {
					if(*data.username != *user->username)
						SendWSMessage(data, adminUser);
				});
				SendOnlineUsersList(*user); // send userlist
			}
			user->admin_connected = false;
			admin_connections_.erase(user);
			break;
		case kSeshID:
			if(args.size() == 1) {
				// The user did not provide an admin session ID
				// so check if their rank is admin
				//if (user->user_rank == UserRank::kAdmin)
			} else if(args.size() == 2) {
				if(!admin_session_id_.empty() && args[1] == admin_session_id_) {
					user->admin_connected = true;
					user->user_rank = UserRank::kAdmin;
					admin_connections_.insert(user);

					// Send login success response
					SendWSMessage(*user, "5.admin,1.0,1.1;");

					if(user->vm_controller != nullptr) {
						// Send new rank to users
						std::string adminUser = "7.adduser,1.1,";
						std::string adminStr = *user->username;
						adminUser += std::to_string(adminStr.length());
						adminUser += ".";
						adminUser += adminStr;
						adminUser += ",1.2;";
						user->vm_controller->GetUsersList().ForEachUser([&](CollabVMUser& data) {
							if(*data.username != *user->username)
								SendWSMessage(data, adminUser);
						});
						SendOnlineUsersList(*user); // send userlist
					}
				} else {
					// Send login failed response
					SendWSMessage(*user, "5.admin,1.0,1.0;");
				}
			}
			break;
		case kMasterPwd:
			if(args.size() == 2 && args[1] == database_.Configuration.MasterPassword) {
				user->admin_connected = true;
				user->user_rank = UserRank::kAdmin;
				admin_connections_.insert(user);

				// Send login success response
				SendWSMessage(*user, "5.admin,1.0,1.1;");

				if(user->vm_controller != nullptr) {
					// Send new rank to users
					std::string adminUser = "7.adduser,1.1,";
					std::string adminStr = *user->username;
					adminUser += std::to_string(adminStr.length());
					adminUser += ".";
					adminUser += adminStr;
					adminUser += ",1.2;";
					user->vm_controller->GetUsersList().ForEachUser([&](CollabVMUser& data) {
						if(*data.username != *user->username)
							SendWSMessage(data, adminUser);
					});
					SendOnlineUsersList(*user); // send userlist
				}
			} else if(args.size() == 2 && args[1] == database_.Configuration.ModPassword && database_.Configuration.ModEnabled) {
				user->user_rank = UserRank::kModerator;

				// Send moderator login success response
				std::string modLogin = "5.admin,1.0,1.3,";
				std::string modPermStr = std::to_string(database_.Configuration.ModPerms);
				modLogin += std::to_string(modPermStr.length());
				modLogin += ".";
				modLogin += modPermStr;
				modLogin += ";";
				SendWSMessage(*user, modLogin);

				if(user->vm_controller != nullptr) {
					// Send new rank to users
					std::string adminUser = "7.adduser,1.1,";
					std::string adminStr = *user->username;
					adminUser += std::to_string(adminStr.length());
					adminUser += ".";
					adminUser += adminStr;
					adminUser += ",1.3;";
					user->vm_controller->GetUsersList().ForEachUser([&](CollabVMUser& data) {
						if(*data.username != *user->username)
							SendWSMessage(data, adminUser);
					});
					SendOnlineUsersList(*user); // send userlist
				}
			} else {
				// Send login failed response
				SendWSMessage(*user, "5.admin,1.0,1.0;");
			}
			break;
		case kGetSettings: {
			std::ostringstream ss("5.admin,1.1,", std::ostringstream::in | std::ostringstream::out | std::ostringstream::ate);
			std::string resp = GetServerSettings();
			ss << resp.length() << '.' << resp << ';';
			SendWSMessage(*user, ss.str());
			break;
		}
		case kSetSettings:
			if(args.size() == 2) {
				std::ostringstream ss("5.admin,1.1,", std::ostringstream::in | std::ostringstream::out | std::ostringstream::ate);
				std::string resp = PerformConfigFunction(args[1]);
				ss << resp.length() << '.' << resp << ';';
				SendWSMessage(*user, ss.str());
			}
			break;
		case kQEMU: {
			if(args.size() != 3)
				break;

			auto it = vm_controllers_.find(args[1]);
			if(it == vm_controllers_.end()) {
				// VM not found
				SendWSMessage(*user, "5.admin,1.0,1.2;");
				break;
			}

			size_t strLen = strlen(args[2]);
			if(!strLen)
				break;

			// TODO: Verify that the VMController is a QEMUController
			// TODO: Implement ^
			std::static_pointer_cast<QEMUController>(it->second)->SendMonitorCommand(std::string(args[2], strLen), std::bind(&CollabVMServer::OnQEMUResponse, shared_from_this(), user, std::placeholders::_1));
			break;
		}
		case kStartController: {
			if(args.size() != 2)
				return;

			std::string vm_name = args[1];
			auto vm_it = vm_controllers_.find(vm_name);
			if(vm_it == vm_controllers_.end()) {
				auto db_it = database_.VirtualMachines.find(vm_name);
				if(db_it != database_.VirtualMachines.end()) {
					CreateVMController(db_it->second)->Start();
					SendWSMessage(*user, "5.admin,1.1,15.{\"result\":true};");
				} else {
					// VM not found
					SendWSMessage(*user, "5.admin,1.0,1.2;");
				}
			} else {
				// VM already running, report success
				SendWSMessage(*user, "5.admin,1.1,15.{\"result\":true};");
			}
			break;
		}
		case kStopController:
		case kRestoreVM:
		case kRebootVM:
		case kResetVM:
		case kRestartVM:
			for(size_t i = 1; i < args.size(); i++) {
				auto it = vm_controllers_.find(args[i]);
				if(it == vm_controllers_.end()) {
					// VM not found
					SendWSMessage(*user, "5.admin,1.0,1.2;");
					continue;
				}

				switch(opcode) {
					//case kStartController:
					//	it->second->Start();
					//	break;
					case kStopController:
						it->second->Stop(VMController::StopReason::kRemove);
						break;
					case kRestoreVM:
						it->second->RestoreVMSnapshot();
						break;
					case kRebootVM:
						it->second->PowerOffVM();
						break;
					case kResetVM:
						it->second->ResetVM();
						break;
					case kRestartVM:
						it->second->Stop(VMController::StopReason::kRestart);
						break;
				}
				// Send success response
				SendWSMessage(*user, "5.admin,1.1,15.{\"result\":true};");
			}
			break;
		case kBanUser:
			if(args.size() == 2 && database_.Configuration.BanCommand != "") {
				for(auto it = connections_.begin(); it != connections_.end(); it++) {
					std::shared_ptr<CollabVMUser> banUser = *it;
					if(!banUser->username)
						continue;
					if(*banUser->username == args[1]) {
						// Replace $IP in ban command with user's IP
						std::string banCmd = database_.Configuration.BanCommand;

						for(size_t it = 0; banCmd.find("$IP", it) != std::string::npos; it = banCmd.find("$IP", it))
							banCmd.replace(banCmd.find("$IP", it), 3, banUser->ip_data.GetIP());

						for(size_t it = 0; banCmd.find("$NAME", it) != std::string::npos; it = banCmd.find("$NAME", it))
							banCmd.replace(banCmd.find("$NAME", it), 5, *banUser->username);

						// Block user's IP
						ExecuteCommandAsync(banCmd);

						// Disconnect user
						PostAction<UserAction>(*banUser, ActionType::kRemoveConnection);
						break;
					}
				}
			}
			break;
		case kForceVote:
			if(user->vm_controller != nullptr) {
				if(args.size() == 1 || (args.size() == 2 && args[1][0] == '0'))
					user->vm_controller->SkipVote(false);
				else if((args.size() == 2 && args[1][0] == '1') && database_.Configuration.ModPerms & 1)
					user->vm_controller->SkipVote(true);
			};
			break;
		case kMuteUser:
			if(args.size() == 3)
				for(auto it = connections_.begin(); it != connections_.end(); it++) {
					std::shared_ptr<CollabVMUser> mutedUser = *it;
					if(!mutedUser->username)
						continue;
					if(*mutedUser->username == args[1]) {
						if(args[2][0] == '2')
							UnmuteUser(mutedUser);
						else
							MuteUser(mutedUser, args[2][0] == '1');
						break;
					}
				}
			break;
		case kKickUser:
			if(args.size() == 2) {
				for(auto it = connections_.begin(); it != connections_.end(); it++) {
					std::shared_ptr<CollabVMUser> kickUser = *it;
					if(!kickUser->username)
						continue;
					if(*kickUser->username == args[1]) {
						// Disconnect user
						PostAction<UserAction>(*kickUser, ActionType::kRemoveConnection);
						break;
					};
				};
			};
			break;
		case kEndUserTurn:
			if(args.size() == 2 && user->vm_controller != nullptr) {
				for(auto it = connections_.begin(); it != connections_.end(); it++) {
					std::shared_ptr<CollabVMUser> endTurnUser = *it;
					if(!endTurnUser->username)
						continue;
					if(*endTurnUser->username == args[1]) {
						user->vm_controller->EndTurn(endTurnUser);
						break;
					};
				};
			};
			break;
		case kClearTurnQueue:
			if(args.size() == 2) {
				auto it = vm_controllers_.find(args[1]);
				if(it != vm_controllers_.end()) {
					it->second->ClearTurnQueue();
				};
			};
			break;
		case kRenameUser:
			if(args.size() == 2 || args.size() == 3) {
				for(auto it = connections_.begin(); it != connections_.end(); it++) {
					std::shared_ptr<CollabVMUser> changeNameUser = *it;
					UsernameChangeResult cnResult = UsernameChangeResult::kSuccess;
					if(!changeNameUser->username)
						continue;
					if(*changeNameUser->username == args[1]) {
						if(args.size() == 2) {
							ChangeUsername(changeNameUser, GenerateUsername(), cnResult, 0);
						} else {
							if(usernames_.find(args[2]) != usernames_.end())
								cnResult = UsernameChangeResult::kUsernameTaken;
							else if(ValidateUsername(args[2])) {
								ChangeUsername(changeNameUser, args[2], cnResult, 0);
							} else {
								cnResult = UsernameChangeResult::kInvalid;
							};
						};
						std::string instr = "5.admin,2.18,1.";
						instr += cnResult;
						instr += ";";
						SendWSMessage(*user, instr);
						break;
					};
				};
			};
			break;
		case kUserIP:
			if(args.size() == 2) {
				for(auto it = connections_.begin(); it != connections_.end(); it++) {
					std::shared_ptr<CollabVMUser> theUser = *it;
					if(!theUser->username)
						continue;
					if(*theUser->username == args[1]) {
						std::string instr = "5.admin,2.19,";
						instr += std::to_string(theUser->username->length());
						instr += ".";
						instr += *theUser->username;
						instr += ",";
						instr += std::to_string(theUser->ip_data.GetIP().length());
						instr += ".";
						instr += theUser->ip_data.GetIP();
						instr += ";";
						SendWSMessage(*user, instr);
						break;
					};
				};
			};
			break;
		case kForceTakeTurn:
			if(user->vm_controller != nullptr && user->username) {
				user->vm_controller->TurnRequest(user, 1, 1);
			};
			break;
	}
}

void CollabVMServer::OnListInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args) {
	std::string instr("4.list");
	for(auto it = vm_controllers_.begin(); it != vm_controllers_.end(); it++) {
		instr += ',';
		const VMSettings& vm_settings = it->second->GetSettings();
		instr += std::to_string(vm_settings.Name.length());
		instr += '.';
		instr += vm_settings.Name;
		instr += ',';
		instr += std::to_string(vm_settings.DisplayName.length());
		instr += '.';
		instr += vm_settings.DisplayName;

		instr += ',';

		std::string* png = it->second->GetThumbnail();
		if(png && png->length()) {
			instr += std::to_string(png->length());
			instr += '.';
			instr += *png;
		} else {
			instr += "0.";
		}
	}
	instr += ';';
	SendWSMessage(*user, instr);
}

void CollabVMServer::OnNopInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args) {
	user->last_nop_instr = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());
}

void CollabVMServer::OnQEMUResponse(std::weak_ptr<CollabVMUser> data, rapidjson::Document& d) {
	auto ptr = data.lock();
	if(!ptr)
		return;

	rapidjson::Value::MemberIterator r = d.FindMember("return");
	if(r != d.MemberEnd() && r->value.IsString() && r->value.GetStringLength() > 0) {
		rapidjson::Value& v = r->value;
		std::string msg = EncodeHTMLString(v.GetString(), v.GetStringLength());
		if(msg.length() < 1)
			return;

		std::ostringstream ss("5.admin,1.2,", std::ostringstream::in | std::ostringstream::out | std::ostringstream::ate);
		ss << msg.length() << '.' << msg << ';';

		server_->send_message(ptr->handle, websocketmm::BuildWebsocketMessage(ss.str()));
	}
}

void CollabVMServer::OnChatInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args) {
	if(args.size() != 1 || !user->username)
		return;

	// Limit message send rate
	auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());
	if(user->ip_data.chat_muted) {
		if((now - user->ip_data.last_chat_msg).count() >= database_.Configuration.ChatMuteTime && user->ip_data.chat_muted == kTempMute)
			user->ip_data.chat_muted = kUnmuted;
		else
			return;
	}

	if(database_.Configuration.ChatRateCount && database_.Configuration.ChatRateTime) {
		// Calculate the time since the user's last message
		if((now - user->ip_data.last_chat_msg).count() < database_.Configuration.ChatRateTime) {
			if(++user->ip_data.chat_msg_count >= database_.Configuration.ChatRateCount) {
				if(user->user_rank == kUnregistered || (user->user_rank == kModerator && !(database_.Configuration.ModPerms & 16))) {
					MuteUser(user, false);
					return;
				}
			}
		} else {
			user->ip_data.chat_msg_count = 0;
			user->ip_data.last_chat_msg = now;
		}
	}

	size_t str_len = strlen(args[0]);
	if(!str_len || str_len > kMaxChatMsgLen)
		return;

	std::string msg = EncodeHTMLString(args[0], str_len);
	if(msg.empty())
		return;

	if(database_.Configuration.ChatMsgHistory) {
		// Add the message to the chat history
		ChatMessage* chat_message = &chat_history_[chat_history_end_];
		chat_message->timestamp = now;
		chat_message->username = user->username;
		chat_message->message = msg;

		uint8_t last_index = database_.Configuration.ChatMsgHistory - 1;

		if(chat_history_end_ == chat_history_begin_ && chat_history_count_) {
			// Increment the begin index
			if(chat_history_begin_ == last_index)
				chat_history_begin_ = 0;
			else
				chat_history_begin_++;
		} else {
			chat_history_count_++;
		}

		// Increment the end index
		if(chat_history_end_ == last_index)
			chat_history_end_ = 0;
		else
			chat_history_end_++;
	}

	std::string instr = "4.chat,";
	instr += std::to_string(user->username->length());
	instr += '.';
	instr += *user->username;
	instr += ',';
	instr += std::to_string(msg.length());
	instr += '.';
	instr += msg;
	instr += ';';

	for(const auto& connection : connections_)
		SendWSMessage(*connection, instr);
}

void CollabVMServer::OnTurnInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args) {
	auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());

	if(user->ip_data.turn_fixed) {
		if((now - user->ip_data.last_turn).count() >= database_.Configuration.TurnMuteTime)
			user->ip_data.turn_fixed = false;
		else
			return;
	}

	// Calculate the time since the user's last turn instruction
	if((now - user->ip_data.last_turn).count() < database_.Configuration.TurnRateTime) {
		if(++user->ip_data.turn_count >= database_.Configuration.TurnRateCount) {
			std::string mute_time = std::to_string(database_.Configuration.TurnMuteTime);
			std::cout << "[Anti-Turnfag] User prevented from taking turns. It has been stopped for " << mute_time << " seconds. IP: " << user->ip_data.GetIP() << std::endl;
			user->ip_data.last_turn = now;
			user->ip_data.turn_fixed = true;
		}
	} else {
		user->ip_data.turn_count = 0;
		//                        user->ip_data.last_turn = now;
	}

	if(user->vm_controller != nullptr && user->username) {
		user->ip_data.last_turn = now;
		// DEBUG
		//std::cout << "Turn count: " << user->ip_data.turn_count << std::endl;
		// END DEBUG
		if(args.size() == 1 && args[0][0] == '0')
			user->vm_controller->EndTurn(user);
		else
			user->vm_controller->TurnRequest(user, 0, user->user_rank == UserRank::kAdmin || (user->user_rank == UserRank::kModerator && database_.Configuration.ModPerms & 64));
	}
}

void CollabVMServer::OnVoteInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args) {
	if(args.size() == 1 && user->vm_controller != nullptr && user->username)
		user->vm_controller->Vote(*user, args[0][0] == '1');
}

void CollabVMServer::OnFileInstruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args) {
	if(!user->vm_controller || args.empty() || args[0][0] == '\0' ||
	   !user->vm_controller->GetSettings().UploadsEnabled)
		return;

	switch(static_cast<ClientFileOp>(args[0][0])) {
		case ClientFileOp::kBegin:
			if(args.size() == 4 && !user->upload_info && !user->ip_data.upload_in_progress) {
				if(SendUploadCooldownTime(*user, *user->vm_controller))
					break;

				std::string filename = args[1];
				if(!IsFilenameValid(filename))
					break;

				uint32_t file_size = std::strtoul(args[2], nullptr, 10);
				bool run_file = args[3][0] == '1';
				if(user->vm_controller->IsFileUploadValid(user, filename, file_size, run_file)) {
					bool found = false;
					std::shared_ptr<VMController> vm;
					for(auto vm_controller : vm_controllers_) {
						if(vm_controller.second.get() == user->vm_controller) {
							vm = vm_controller.second;
							found = true;
							break;
						}
					}
					assert(found);

					user->upload_info = std::make_shared<UploadInfo>(user, vm, filename, file_size, run_file,
																	 user->vm_controller->GetSettings().UploadCooldownTime);

					std::unique_lock<std::mutex> lock(upload_lock_);

					user->upload_info->upload_it = upload_ids_.end();

					lock.unlock();

					user->ip_data.upload_in_progress = true;

					if(upload_count_ != kMaxFileUploads) {
						StartFileUpload(*user);
					} else {
						user->waiting_for_upload = true;
						upload_queue_.push_back(user);
					}
					break;
				}
			}
			SendWSMessage(*user, "4.file,1.5;");
			break;
	}
}

void CollabVMServer::OnAgentConnect(const std::shared_ptr<VMController>& controller,
									const std::string& os_name, const std::string& service_pack,
									const std::string& pc_name, const std::string& username, uint32_t max_filename) {
	PostAction<AgentConnectAction>(controller, os_name, service_pack, pc_name, username, max_filename);
}

void CollabVMServer::OnAgentDisconnect(const std::shared_ptr<VMController>& controller) {
	PostAction<VMAction>(controller, ActionType::kAgentDisconnect);
}

// TODO

//void CollabVMServer::OnAgentHeartbeatTimeout(const std::shared_ptr<VMController>& controller) {
//	PostAction<VMAction>(controller, ActionType::kHeartbeatTimeout);
//}

bool CollabVMServer::IsFilenameValid(const std::string& filename) {
	for(char c : filename) {
		// Only allow printable ASCII characters
		if(c < ' ' || c > '~')
			return false;
		// Characters disallowed by Windows
		switch(c) {
			case '<':
			case '>':
			case ':':
			case '"':
			case '/':
			case '\\':
			case '|':
			case '?':
			case '*':
				return false;
		}
	}
	return true;
}

void CollabVMServer::OnUploadTimeout(const boost::system::error_code ec, std::shared_ptr<UploadInfo> upload_info) {
	/*
	if (ec)
		return;

	UploadInfo::HttpUploadState prev_state =
		upload_info->http_state.exchange(UploadInfo::HttpUploadState::kCancel);
	if (prev_state == UploadInfo::HttpUploadState::kNotStarted)
	{
		std::unique_lock<std::mutex> lock(process_queue_lock_);
		process_queue_.push(new HttpAction(ActionType::kHttpUploadTimedout, upload_info));
		lock.unlock();
		process_wait_.notify_one();
	}
	else if (prev_state == UploadInfo::HttpUploadState::kNotWriting)
	{
		std::unique_lock<std::mutex> lock(process_queue_lock_);
		process_queue_.push(new HttpAction(ActionType::kHttpUploadFailed, upload_info));
		lock.unlock();
		process_wait_.notify_one();
	}
     */
}

void CollabVMServer::StartFileUpload(CollabVMUser& user) {
	/*
	assert(user.upload_info);
	const std::shared_ptr<UploadInfo>& upload_info = user.upload_info;
	std::string file_path = kFileUploadPath;
	file_path += std::to_string(upload_count_ + 1);

	upload_info->file_stream.open(file_path, std::fstream::in | std::fstream::out | std::fstream::trunc | std::fstream::binary);
	if (upload_info->file_stream.is_open() && upload_info->file_stream.good())
	{
		upload_count_++;
		upload_info->file_path = file_path;

		boost::asio::steady_timer* timer = new boost::asio::steady_timer(service_);
		std::string upload_id = GenerateUuid();
		unique_lock<std::mutex> lock(upload_lock_);
		auto result = upload_ids_.insert({ upload_id, upload_info });
		while (!result.second)
		{
			upload_id = GenerateUuid();
			result = upload_ids_.insert({ upload_id, upload_info });
		}
		upload_info->upload_it = result.first;
		upload_info->timeout_timer = timer;
		lock.unlock();

		std::string instr = "4.file,1.0,";
		instr += std::to_string(upload_id.length());
		instr += '.';
		instr += upload_id;
		instr += ';';
		SendWSMessage(user, instr);

		boost::system::error_code ec;
		timer->expires_from_now(std::chrono::seconds(kUploadStartTimeout), ec);
		timer->async_wait(std::bind(&CollabVMServer::OnUploadTimeout, shared_from_this(),
									std::placeholders::_1, upload_info));
	}
	else
	{
		CancelFileUpload(user);
		SendWSMessage(user, "4.file,1.5;");

		std::cout << "Error: Failed to create file \"" << file_path << "\" for upload." << std::endl;
	}
     */
}

void CollabVMServer::SendUploadResultToIP(IPData& ip_data, const CollabVMUser& user, const std::string& instr) {
	for(const std::shared_ptr<CollabVMUser>& connection : connections_)
		if(&connection->ip_data == &ip_data && connection->vm_controller && connection.get() != &user)
			SendWSMessage(*connection, instr);
}

static void AppendCooldownTime(std::string& instr, uint32_t cooldown_time) {
	std::string temp = std::to_string(cooldown_time * 1000);
	instr += std::to_string(temp.length());
	instr += '.';
	instr += temp;
	instr += ';';
}

void CollabVMServer::FileUploadEnded(const std::shared_ptr<UploadInfo>& upload_info,
									 const std::shared_ptr<CollabVMUser>* user, FileUploadResult result) {
	/*
	if (boost::asio::steady_timer* timer = upload_info->timeout_timer)
	{
		upload_info->timeout_timer = nullptr;
		boost::system::error_code ec;
		timer->cancel(ec);
		delete timer;
	}

	VMController& vm_controller = *upload_info->vm_controller;
	if (user)
	{
		upload_info->file_stream.close();
		std::remove(upload_info->file_path.c_str());

		uint32_t cooldown_time = vm_controller.GetSettings().UploadCooldownTime;
		// The IPData has at least one client associated with it so
		// locking isn't required
		SetUploadCooldownTime(upload_info->ip_data, cooldown_time);

		std::string uploader_instr;
		std::string other_instr;
		switch (result)
		{
		case FileUploadResult::kHttpUploadTimedOut:
			uploader_instr = "4.file,1.7";
			goto send_message;
		case FileUploadResult::kHttpUploadFailed:
			uploader_instr = "4.file,1.5";
			goto send_message;
		case FileUploadResult::kAgentUploadSucceeded:
			uploader_instr = "4.file,1.2";
		send_message:
			if (cooldown_time)
			{
				std::string cooldown_str;
				AppendCooldownTime(cooldown_str, cooldown_time);

				uploader_instr += ',';
				uploader_instr += cooldown_str;

				other_instr = "4.file,1.4,";
				other_instr += cooldown_str;
			}
			else
			{
				uploader_instr += ';';
				other_instr = "4.file,1.4,1.0;";
			}
			SendWSMessage(**user, uploader_instr);
			break;
		}

		if (upload_info->ip_data.connections > 1)
			SendUploadResultToIP(upload_info->ip_data, *user->get(), other_instr);
	}
	else
	{
		// The IPData might not have any clients associated with it so
		// locking is required
		unique_lock<std::mutex> lock(ip_lock_);
		uint32_t cooldown_time = vm_controller.GetSettings().UploadCooldownTime;
		SetUploadCooldownTime(upload_info->ip_data, cooldown_time);
		if (upload_info->ip_data.connections > 1)
		{
			std::string instr = cooldown_time ? "4.file,1.4," : "4.file,1.4,1.0;";
			if (cooldown_time)
				AppendCooldownTime(instr, cooldown_time);
			SendUploadResultToIP(upload_info->ip_data, *user->get(), instr);
		}
		lock.unlock();
	}

	switch (result)
	{
	case FileUploadResult::kHttpUploadTimedOut:
	case FileUploadResult::kHttpUploadFailed:
		StartNextUpload();
		break;
	case FileUploadResult::kAgentUploadSucceeded:
		BroadcastUploadedFileInfo(*upload_info, vm_controller);
		// Fall through
	case FileUploadResult::kAgentUploadFailed:
		if (vm_controller.agent_upload_queue_.empty())
		{
			vm_controller.agent_upload_in_progress_ = false;
			StartNextUpload();
		}
		else
		{
			vm_controller.UploadFile(vm_controller.agent_upload_queue_.front());
			vm_controller.agent_upload_queue_.pop_front();
		}
		break;
	}
     */
}

void CollabVMServer::StartNextUpload() {
	upload_count_--;
	if(!upload_queue_.empty()) {
		CollabVMUser& user = *upload_queue_.front();
		upload_queue_.pop_front();
		user.waiting_for_upload = false;
		StartFileUpload(user);
	}
}

void CollabVMServer::CancelFileUpload(CollabVMUser& user) {
	if(!user.upload_info)
		return;

	UploadInfo::HttpUploadState prev_state =
	user.upload_info->http_state.exchange(UploadInfo::HttpUploadState::kCancel);
	if(prev_state != UploadInfo::HttpUploadState::kNotStarted &&
	   prev_state != UploadInfo::HttpUploadState::kNotWriting)
		return;

	if(prev_state == UploadInfo::HttpUploadState::kNotStarted) {
		std::lock_guard<std::mutex> lock(upload_lock_);
		if(user.upload_info->upload_it != upload_ids_.end())
			upload_ids_.erase(user.upload_info->upload_it);
	}

	if(boost::asio::steady_timer* timer = user.upload_info->timeout_timer) {
		user.upload_info->timeout_timer = nullptr;
		boost::system::error_code ec;
		timer->cancel(ec);
		delete timer;
	}

	uint32_t cooldown_time = user.vm_controller->GetSettings().UploadCooldownTime;
	SetUploadCooldownTime(user.ip_data, cooldown_time);

	if(user.upload_info->ip_data.connections > 1) {
		std::string instr = cooldown_time ? "4.file,1.4," : "4.file,1.4,1.0;";
		if(cooldown_time)
			AppendCooldownTime(instr, cooldown_time);
		SendUploadResultToIP(user.upload_info->ip_data, user, instr);
	}

	// Close the fstream first to allow a new one to be opened if there is a pending upload
	user.upload_info->file_stream.close();
	user.upload_info.reset();

	if(user.waiting_for_upload) {
		user.waiting_for_upload = false;
		bool user_found = false;
		for(auto it = upload_queue_.begin(); it != upload_queue_.end(); it++) {
			if((*it).get() == &user) {
				upload_queue_.erase(it);
				user_found = true;
				break;
			}
		}
		assert(user_found);
	} else {
		StartNextUpload();
	}
}

void CollabVMServer::SetUploadCooldownTime(IPData& ip_data, uint32_t time) {
	ip_data.upload_in_progress = false;
	if(time)
		ip_data.next_upload_time = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now()) + std::chrono::seconds(time);
}

bool CollabVMServer::SendUploadCooldownTime(CollabVMUser& user, const VMController& vm_controller) {
	if(vm_controller.GetSettings().UploadCooldownTime) {
		using namespace std::chrono;
		int64_t remaining = (user.ip_data.next_upload_time -
							 time_point_cast<milliseconds>(steady_clock::now()))
							.count();

		if(remaining > 0) {
			std::string instr = "4.file,1.4,";
			std::string temp = std::to_string(remaining);
			instr += std::to_string(temp.length());
			instr += '.';
			instr += temp;
			instr += ';';
			SendWSMessage(user, instr);
			return true;
		}
	}

	return false;
}

void CollabVMServer::BroadcastUploadedFileInfo(UploadInfo& upload_info, VMController& vm_controller) {
#define UPLOAD_STR " uploaded "
#define FILE_SIZE_PREFIX " ("
#define FILE_SIZE_SUFFIX " bytes)"
	std::string instr = "4.chat,0.,";
	std::string file_size = std::to_string(upload_info.file_size);
	std::string escaped_filename = EncodeHTMLString(upload_info.filename.c_str(), upload_info.filename.length());
	instr += std::to_string(upload_info.username->length() + STR_LEN(UPLOAD_STR) + escaped_filename.length() +
							STR_LEN(FILE_SIZE_PREFIX) + file_size.length() + STR_LEN(FILE_SIZE_SUFFIX));
	instr += '.';
	instr += *upload_info.username;
	instr += UPLOAD_STR;
	instr += escaped_filename;
	instr += FILE_SIZE_PREFIX;
	instr += file_size;
	instr += FILE_SIZE_SUFFIX;
	instr += ';';

	vm_controller.GetUsersList().ForEachUser([&](CollabVMUser& user) {
		SendWSMessage(user, instr);
	});
}

void CollabVMServer::OnFileUploadFailed(const std::shared_ptr<VMController>& controller, const std::shared_ptr<UploadInfo>& info) {
	assert(controller == info->vm_controller);

	PostAction<FileUploadAction>(info, FileUploadAction::UploadResult::kFailed);
}

void CollabVMServer::OnFileUploadFinished(const std::shared_ptr<VMController>& controller, const std::shared_ptr<UploadInfo>& info) {
	assert(controller == info->vm_controller);

	PostAction<FileUploadAction>(info, FileUploadAction::UploadResult::kSuccess);
}

void CollabVMServer::OnFileUploadExecFinished(const std::shared_ptr<VMController>& controller, const std::shared_ptr<UploadInfo>& info, bool exec_success) {
	assert(controller == info->vm_controller);
	// TODO: Send exec_success

	PostAction<FileUploadAction>(info, FileUploadAction::UploadResult::kSuccess);
}

/**
 * The error message that the server will respond with when
 * it receives a JSON string with an value of the wrong type.
 */
static const std::string invalid_object_ = "Invalid object type";

/**
 * Simple convenience function for writing an object
 * to a JSON string.
 */
static void WriteJSONObject(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::string key, const std::string val) {
	writer.StartObject();
	writer.String(key.c_str());
	writer.String(val.c_str());
	writer.EndObject();
}

/**
 * Parses the JSON object and changes the setings of the VMSettings.
 * @returns Whether the settings were valid
 */
bool CollabVMServer::ParseVMSettings(VMSettings& vm, rapidjson::Value& settings, rapidjson::Writer<rapidjson::StringBuffer>& writer) {
	bool valid = true;
	for(auto it = settings.MemberBegin(); it != settings.MemberEnd(); it++) {
		for(size_t i = 0; i < sizeof(vm_settings_) / sizeof(std::string); i++) {
			std::string key(it->name.GetString(), it->name.GetStringLength());
			if(key == vm_settings_[i]) {
				rapidjson::Value& value = it->value;
				switch(i) {
					case kName:
						if(value.IsString() && value.GetStringLength() > 0) {
							vm.Name = std::string(value.GetString(), value.GetStringLength());
						} else {
							WriteJSONObject(writer, vm_settings_[kName], invalid_object_);
							valid = false;
						}
						break;
					case kAutoStart:
						if(value.IsBool()) {
							vm.AutoStart = value.GetBool();
						} else {
							WriteJSONObject(writer, vm_settings_[kAutoStart], invalid_object_);
							valid = false;
						}
						break;
					case kDisplayName:
						if(value.IsString()) {
							vm.DisplayName = std::string(value.GetString(), value.GetStringLength());
						} else {
							WriteJSONObject(writer, vm_settings_[kDisplayName], invalid_object_);
							valid = false;
						}
						break;
					case kHypervisor: {
						if(!value.IsString()) {
							WriteJSONObject(writer, vm_settings_[kHypervisor], invalid_object_);
							valid = false;
						}
						std::string hypervisor = std::string(value.GetString(), value.GetStringLength());
						for(size_t x = 0; x < sizeof(hypervisor_names_) / sizeof(std::string); x++) {
							if(hypervisor == hypervisor_names_[x]) {
								vm.Hypervisor = static_cast<VMSettings::HypervisorEnum>(x);
								goto found_hypervisor;
							}
						}
						// Hypervisor name not recognized
						WriteJSONObject(writer, vm_settings_[kHypervisor], "Unknown hypervisor name");
						valid = false;
					found_hypervisor:
						break;
					}
					case kRestoreShutdown:
						if(value.IsBool()) {
							vm.RestoreOnShutdown = value.GetBool();
						} else {
							WriteJSONObject(writer, vm_settings_[kRestoreShutdown], invalid_object_);
							valid = false;
						}
						break;
					case kRestoreTimeout:
						if(value.IsBool()) {
							vm.RestoreOnTimeout = value.GetBool();
						} else {
							WriteJSONObject(writer, vm_settings_[kRestoreTimeout], invalid_object_);
							valid = false;
						}
						break;
					case kVNCAddress:
						if(value.IsString()) {
							vm.VNCAddress = std::string(value.GetString(), value.GetStringLength());
						} else {
							WriteJSONObject(writer, vm_settings_[kVNCAddress], invalid_object_);
							valid = false;
						}
						break;
					case kVNCPort:
						if(value.IsUint()) {
							if(value.GetUint() < std::numeric_limits<uint16_t>::max()) {
								vm.VNCPort = value.GetUint();
							} else {
								WriteJSONObject(writer, vm_settings_[kVNCPort], "Port too large");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, vm_settings_[kVNCPort], invalid_object_);
							valid = false;
						}
						break;
					case kQMPSocketType:
						if(value.IsString() && value.GetStringLength()) {
							std::string str(value.GetString(), value.GetStringLength());
							if(str == "tcp")
								vm.QMPSocketType = VMSettings::SocketType::kTCP;
							else if(str == "local")
								vm.QMPSocketType = VMSettings::SocketType::kLocal;
							else {
								WriteJSONObject(writer, vm_settings_[kQMPSocketType], "Invalid socket type");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, vm_settings_[kQMPSocketType], invalid_object_);
							valid = false;
						}
						break;
					case kQMPAddress:
						if(value.IsString())
							vm.QMPAddress = std::string(value.GetString(), value.GetStringLength());
						else {
							WriteJSONObject(writer, vm_settings_[kQMPAddress], invalid_object_);
							valid = false;
						}
						break;
					case kQMPPort:
						if(value.IsUint()) {
							if(value.GetUint() < std::numeric_limits<uint16_t>::max()) {
								vm.QMPPort = value.GetUint();
							} else {
								WriteJSONObject(writer, vm_settings_[kQMPPort], "Port too large");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, vm_settings_[kQMPPort], invalid_object_);
							valid = false;
						}
						break;
					case kMaxAttempts:
						if(value.IsUint()) {
							if(value.GetUint() < std::numeric_limits<uint8_t>::max()) {
								vm.MaxAttempts = value.GetUint();
							} else {
								WriteJSONObject(writer, vm_settings_[kMaxAttempts], "Maximum connection attempts too large");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, vm_settings_[kMaxAttempts], invalid_object_);
							valid = false;
						}
						break;
					case kQEMUCmd:
						if(value.IsString()) {
							vm.QEMUCmd = std::string(value.GetString(), value.GetStringLength());
						} else {
							WriteJSONObject(writer, vm_settings_[kQEMUCmd], invalid_object_);
							valid = false;
						}
						break;
					case kQEMUSnapshotMode:
						if(value.IsString()) {
							std::string mode = std::string(value.GetString(), value.GetStringLength());
							for(size_t x = 0; x < sizeof(snapshot_modes_) / sizeof(std::string); x++) {
								if(mode == snapshot_modes_[x]) {
									vm.QEMUSnapshotMode = (VMSettings::SnapshotMode)x;
									goto found_mode;
								}
							}
							WriteJSONObject(writer, vm_settings_[kQEMUSnapshotMode], "Unknown snapshot mode");
							valid = false;
						found_mode:
							break;
						} else {
							WriteJSONObject(writer, vm_settings_[kQEMUSnapshotMode], invalid_object_);
							valid = false;
						}
						break;
					case kTurnsEnabled:
						if(value.IsBool()) {
							vm.TurnsEnabled = value.GetBool();
							// TODO
							//if (!config.TurnsEnabled)
							//{
							//	current_turn_ = nullptr;
							//	turn_queue_.clear();
							//	boost::system::error_code ec;
							//	turn_timer_.cancel(ec);
							//	BroadcastTurnInfo();
							//}
						} else {
							WriteJSONObject(writer, server_settings_[kTurnsEnabled], invalid_object_);
							valid = false;
						}
						break;
					case kTurnTime:
						if(value.IsUint()) {
							vm.TurnTime = value.GetUint();
						} else {
							WriteJSONObject(writer, server_settings_[kTurnTime], invalid_object_);
							valid = false;
						}
						break;
					case kVotesEnabled:
						if(value.IsBool()) {
							vm.VotesEnabled = value.GetBool();
						} else {
							WriteJSONObject(writer, server_settings_[kVotesEnabled], invalid_object_);
							valid = false;
						}
						break;
					case kVoteTime:
						if(value.IsUint()) {
							vm.VoteTime = value.GetUint();
						} else {
							WriteJSONObject(writer, server_settings_[kVoteTime], invalid_object_);
							valid = false;
						}
						break;
					case kVoteCooldownTime:
						if(value.IsUint()) {
							vm.VoteCooldownTime = value.GetUint();
						} else {
							WriteJSONObject(writer, server_settings_[kVoteCooldownTime], invalid_object_);
							valid = false;
						}
						break;
					case kAgentEnabled:
						if(value.IsBool()) {
							vm.AgentEnabled = value.GetBool();
						} else {
							WriteJSONObject(writer, server_settings_[kAgentEnabled], invalid_object_);
							valid = false;
						}
						break;
					case kAgentSocketType:
						if(value.IsString() && value.GetStringLength()) {
							std::string str(value.GetString(), value.GetStringLength());
							if(str == "tcp")
								vm.AgentSocketType = VMSettings::SocketType::kTCP;
							else if(str == "local")
								vm.AgentSocketType = VMSettings::SocketType::kLocal;
							else {
								WriteJSONObject(writer, vm_settings_[kAgentSocketType], "Invalid socket type");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, vm_settings_[kAgentSocketType], invalid_object_);
							valid = false;
						}
						break;
					case kAgentUseVirtio:
						if(value.IsBool()) {
							vm.AgentUseVirtio = value.GetBool();
						} else {
							WriteJSONObject(writer, server_settings_[kAgentUseVirtio], invalid_object_);
							valid = false;
						}
						break;
					case kAgentAddress:
						if(value.IsString()) {
							vm.AgentAddress = std::string(value.GetString(), value.GetStringLength());
						} else {
							WriteJSONObject(writer, vm_settings_[kAgentAddress], invalid_object_);
							valid = false;
						}
						break;
					case kAgentPort:
						if(value.IsUint()) {
							if(value.GetUint() < std::numeric_limits<uint16_t>::max()) {
								vm.AgentPort = value.GetUint();
							} else {
								WriteJSONObject(writer, vm_settings_[kAgentPort], "Port too large");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, vm_settings_[kAgentPort], invalid_object_);
							valid = false;
						}
						break;
					case kRestoreHeartbeat:
						if(value.IsBool()) {
							vm.RestoreHeartbeat = value.GetBool();
						} else {
							WriteJSONObject(writer, server_settings_[kRestoreHeartbeat], invalid_object_);
							valid = false;
						}
						break;
					case kHeartbeatTimeout:
						if(value.IsUint()) {
							//vm.HeartbeatTimeout = value.GetUint();
						} else {
							WriteJSONObject(writer, server_settings_[kHeartbeatTimeout], invalid_object_);
							valid = false;
						}
						break;
					case kUploadsEnabled:
						if(value.IsBool()) {
							vm.UploadsEnabled = value.GetBool();
						} else {
							WriteJSONObject(writer, server_settings_[kUploadsEnabled], invalid_object_);
							valid = false;
						}
						break;
					case kUploadCooldownTime:
						if(value.IsUint()) {
							vm.UploadCooldownTime = value.GetUint();
						} else {
							WriteJSONObject(writer, server_settings_[kUploadCooldownTime], invalid_object_);
							valid = false;
						}
						break;
					case kUploadMaxSize:
						if(value.IsUint()) {
							vm.MaxUploadSize = value.GetUint();
						} else {
							WriteJSONObject(writer, server_settings_[kUploadMaxSize], invalid_object_);
							valid = false;
						}
						break;
					case kUploadMaxFilename:
						if(value.IsUint()) {
							if(value.GetUint() < std::numeric_limits<uint8_t>::max()) {
								vm.UploadMaxFilename = value.GetUint();
							} else {
								WriteJSONObject(writer, vm_settings_[kUploadMaxFilename], "Max filename is too long");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, server_settings_[kUploadMaxFilename], invalid_object_);
							valid = false;
						}
						break;
					case kMOTD:
						if(value.IsString()) {
							vm.MOTD = std::string(value.GetString(), value.GetStringLength());
						} else {
							WriteJSONObject(writer, vm_settings_[kMOTD], invalid_object_);
							valid = false;
						}
						break;
				}
				break;
			}
		}
	}
	if(valid) {
		// Set the value of the "result" property to true to indicate success
		writer.Bool(true);
	}
	return valid;
}

std::string CollabVMServer::PerformConfigFunction(const std::string& json) {
	// Start the JSON response
	rapidjson::StringBuffer str_buf;
	rapidjson::Writer<rapidjson::StringBuffer> writer(str_buf);
	// Start the root object
	writer.StartObject();
	writer.String("result");
	if(json.empty()) {
		writer.String("Request was empty");
		writer.EndObject();
		return std::string(str_buf.GetString(), str_buf.GetSize());
	}
	// Copy the data to a writable buffer
	std::string buf(json);

	// Parse the JSON
	Document d;
	d.ParseInsitu(buf.data());

	if(!d.IsObject()) {
		writer.String("Root value was not an object");
		writer.EndObject();
		return std::string(str_buf.GetString(), str_buf.GetSize());
	}

	for(auto it = d.MemberBegin(); it != d.MemberEnd(); it++) {
		for(size_t i = 0; i < sizeof(config_functions_) / sizeof(std::string); i++) {
			std::string key = std::string(it->name.GetString(), it->name.GetStringLength());
			if(key == config_functions_[i]) {
				rapidjson::Value& value = it->value;
				switch(i) {
					case kSettings:
						ParseServerSettings(value, writer);
						break;
					case kPassword:
						if(value.IsString()) {
							database_.Configuration.MasterPassword = std::string(value.GetString(), value.GetStringLength());
							database_.Save(database_.Configuration);
							writer.Bool(true);
						} else {
							WriteJSONObject(writer, config_functions_[kPassword], invalid_object_);
						}
						break;
					case kModPassword:
						if(value.IsString()) {
							database_.Configuration.ModPassword = std::string(value.GetString(), value.GetStringLength());
							database_.Save(database_.Configuration);
							writer.Bool(true);
						} else {
							WriteJSONObject(writer, config_functions_[kModPassword], invalid_object_);
						}
						break;
					case kAddVM: {
						if(!value.IsObject()) {
							WriteJSONObject(writer, config_functions_[kAddVM], invalid_object_);
							break;
						}
						// Get the name of the VM to update
						auto name_it = value.FindMember("name");
						if(name_it == value.MemberEnd()) {
							WriteJSONObject(writer, config_functions_[kUpdateVM], "No VM name");
							break;
						}

						if(!name_it->value.IsString()) {
							WriteJSONObject(writer, config_functions_[kUpdateVM], invalid_object_);
							break;
						}

						if(!name_it->value.GetStringLength()) {
							WriteJSONObject(writer, config_functions_[kUpdateVM], "VM name cannot be empty");
							break;
						}

						// Check if the VM name is already taken
						auto vm_it = database_.VirtualMachines.find(std::string(name_it->value.GetString(), name_it->value.GetStringLength()));
						if(vm_it == database_.VirtualMachines.end()) {
							std::shared_ptr<VMSettings> vm = std::make_shared<VMSettings>();

							if(ParseVMSettings(*vm, it->value, writer)) {
								database_.AddVM(vm);

								// If the VMSettings are configured to autostart
								if(vm->AutoStart)
									CreateVMController(vm)->Start();

								WriteServerSettings(writer);
								break;
							}
						} else {
							writer.String("A VM with that name already exists");
						}
						break;
					}
					case kUpdateVM: {
						if(!value.IsObject()) {
							WriteJSONObject(writer, config_functions_[kUpdateVM], invalid_object_);
							break;
						}
						// Get the name of the VM to update
						auto name_it = value.FindMember("name");
						if(name_it == value.MemberEnd()) {
							WriteJSONObject(writer, config_functions_[kUpdateVM], "No VM name");
							break;
						}

						if(!name_it->value.IsString()) {
							WriteJSONObject(writer, config_functions_[kUpdateVM], invalid_object_);
							break;
						}
						// Find the VM with the corresponding name
						auto vm_it = database_.VirtualMachines.find(std::string(name_it->value.GetString(), name_it->value.GetStringLength()));
						if(vm_it == database_.VirtualMachines.end()) {
							WriteJSONObject(writer, config_functions_[kUpdateVM], "Unknown VM name");
							break;
						}

						std::shared_ptr<VMSettings> vm = std::make_shared<VMSettings>(*vm_it->second);
						if(ParseVMSettings(*vm, it->value, writer)) {
							database_.UpdateVM(vm);
							vm_it->second = vm;

							auto vm_it = vm_controllers_.find(vm->Name);
							if(vm_it != vm_controllers_.end())
								vm_it->second->ChangeSettings(vm);

							WriteServerSettings(writer);
						}
						break;
					}
					case kDeleteVM: {
						if(!value.IsString()) {
							WriteJSONObject(writer, config_functions_[kDeleteVM], invalid_object_);
							break;
						}
						std::string vm_name(value.GetString(), value.GetStringLength());

						// Find the VM with the corresponding name
						auto vm_it = database_.VirtualMachines.find(vm_name);
						if(vm_it == database_.VirtualMachines.end()) {
							// VM with ID not found
							writer.String("VM with ID not found");
							break;
						}
						std::map<std::string, std::shared_ptr<VMController>>::iterator vm_ctrl_it;
						if((vm_ctrl_it = vm_controllers_.find(vm_name)) != vm_controllers_.end()) {
							vm_ctrl_it->second->Stop(VMController::StopReason::kRemove);
						}

						database_.RemoveVM(vm_it->first);

						writer.Bool(true);

						WriteServerSettings(writer);
						break;
					}
					default:
						writer.String("Unsupported operation");
				}
				break;
			}
		}
	}

	// End the root object
	writer.EndObject();

	return std::string(str_buf.GetString(), str_buf.GetSize());
}

std::string CollabVMServer::GetServerSettings() {
	rapidjson::StringBuffer str_buf;
	rapidjson::Writer<rapidjson::StringBuffer> writer(str_buf);
	writer.StartObject();
	WriteServerSettings(writer);
	writer.EndObject();
	return std::string(str_buf.GetString(), str_buf.GetSize());
}

void CollabVMServer::WriteServerSettings(rapidjson::Writer<rapidjson::StringBuffer>& writer) {
	writer.String("settings");
	writer.StartObject();

	writer.String(server_settings_[kChatRateCount].c_str());
	writer.Uint(database_.Configuration.ChatRateCount);

	writer.String(server_settings_[kChatRateTime].c_str());
	writer.Uint(database_.Configuration.ChatRateTime);

	writer.String(server_settings_[kChatMuteTime].c_str());
	writer.Uint(database_.Configuration.ChatMuteTime);

	writer.String(server_settings_[kMaxConnections].c_str());
	writer.Uint(database_.Configuration.MaxConnections);

	writer.String(server_settings_[kMaxUploadTime].c_str());
	writer.Uint(database_.Configuration.MaxUploadTime);

	writer.String(server_settings_[kBanCommand].c_str());
	writer.String(database_.Configuration.BanCommand.c_str());

	writer.String(server_settings_[kJPEGQuality].c_str());
	writer.Uint(database_.Configuration.JPEGQuality);

	writer.String(server_settings_[kModEnabled].c_str());
	writer.Bool(database_.Configuration.ModEnabled);

	writer.String(server_settings_[kModPerms].c_str());
	writer.Uint(database_.Configuration.ModPerms);

	// "vm" is an array of objects containing the settings for each VM
	writer.String("vm");
	writer.StartArray();

	for(auto it = database_.VirtualMachines.begin(); it != database_.VirtualMachines.end(); it++) {
		std::shared_ptr<VMSettings>& vm = it->second;
		writer.StartObject();
		writer.String("name");
		writer.String(vm->Name.c_str());
		for(size_t i = 0; i < sizeof(vm_settings_) / sizeof(std::string); i++) {
			switch(i) {
				case kAutoStart:
					writer.String(vm_settings_[kAutoStart].c_str());
					writer.Bool(vm->AutoStart);
					break;
				case kDisplayName:
					writer.String(vm_settings_[kDisplayName].c_str());
					writer.String(vm->DisplayName.c_str());
					break;
				case kHypervisor:
					writer.String(vm_settings_[kHypervisor].c_str());
					writer.String(hypervisor_names_[static_cast<size_t>(vm->Hypervisor)].c_str());
					break;
				case kStatus: {
					writer.String(vm_settings_[kStatus].c_str());
					auto vm_it = vm_controllers_.find(vm->Name);
					if(vm_it != vm_controllers_.end()) {
						writer.Uint(static_cast<uint32_t>(vm_it->second->GetState()));
						break;
					}
					writer.Uint(static_cast<uint32_t>(VMController::ControllerState::kStopped));
					break;
				}
				case kRestoreShutdown:
					writer.String(vm_settings_[kRestoreShutdown].c_str());
					writer.Bool(vm->RestoreOnShutdown);
					break;
				case kRestoreTimeout:
					writer.String(vm_settings_[kRestoreTimeout].c_str());
					writer.Bool(vm->RestoreOnTimeout);
					break;
				case kVNCAddress:
					writer.String(vm_settings_[kVNCAddress].c_str());
					writer.String(vm->VNCAddress.c_str());
					break;
				case kVNCPort:
					writer.String(vm_settings_[kVNCPort].c_str());
					writer.Uint(vm->VNCPort);
					break;
				case kQMPSocketType:
					writer.String(vm_settings_[kQMPSocketType].c_str());
					writer.String(vm->QMPSocketType == VMSettings::SocketType::kLocal ? "local" : "tcp");
					break;
				case kQMPAddress:
					writer.String(vm_settings_[kQMPAddress].c_str());
					writer.String(vm->QMPAddress.c_str());
					break;
				case kQMPPort:
					writer.String(vm_settings_[kQMPPort].c_str());
					writer.Uint(vm->QMPPort);
					break;
				case kMaxAttempts:
					writer.String(vm_settings_[kMaxAttempts].c_str());
					writer.Uint(vm->MaxAttempts);
					break;
				case kQEMUCmd:
					writer.String(vm_settings_[kQEMUCmd].c_str());
					writer.String(vm->QEMUCmd.c_str());
					break;
				case kQEMUSnapshotMode:
					writer.String(vm_settings_[kQEMUSnapshotMode].c_str());
					writer.String(snapshot_modes_[static_cast<size_t>(vm->QEMUSnapshotMode)].c_str());
					break;
				case kTurnsEnabled:
					writer.String(vm_settings_[kTurnsEnabled].c_str());
					writer.Bool(vm->TurnsEnabled);
					break;
				case kTurnTime:
					writer.String(vm_settings_[kTurnTime].c_str());
					writer.Uint(vm->TurnTime);
					break;
				case kVotesEnabled:
					writer.String(vm_settings_[kVotesEnabled].c_str());
					writer.Bool(vm->VotesEnabled);
					break;
				case kVoteTime:
					writer.String(vm_settings_[kVoteTime].c_str());
					writer.Uint(vm->VoteTime);
					break;
				case kVoteCooldownTime:
					writer.String(vm_settings_[kVoteCooldownTime].c_str());
					writer.Uint(vm->VoteCooldownTime);
					break;
				case kAgentEnabled:
					writer.String(vm_settings_[kAgentEnabled].c_str());
					writer.Bool(vm->AgentEnabled);
					break;
				case kAgentSocketType:
					writer.String(vm_settings_[kAgentSocketType].c_str());
					writer.String(vm->AgentSocketType == VMSettings::SocketType::kLocal ? "local" : "tcp");
					break;
				case kAgentUseVirtio:
					writer.String(vm_settings_[kAgentUseVirtio].c_str());
					writer.Bool(vm->AgentUseVirtio);
					break;
				case kAgentAddress:
					writer.String(vm_settings_[kAgentAddress].c_str());
					writer.String(vm->AgentAddress.c_str());
					break;
				case kAgentPort:
					writer.String(vm_settings_[kAgentPort].c_str());
					writer.Uint(vm->AgentPort);
					break;
				case kRestoreHeartbeat:
					writer.String(vm_settings_[kRestoreHeartbeat].c_str());
					writer.Bool(vm->RestoreHeartbeat);
					break;
				case kHeartbeatTimeout:
					writer.String(vm_settings_[kHeartbeatTimeout].c_str());
					//writer.Uint(vm->HeartbeatTimeout);
					writer.Uint(0);
					break;
				case kUploadsEnabled:
					writer.String(vm_settings_[kUploadsEnabled].c_str());
					writer.Bool(vm->UploadsEnabled);
					break;
				case kUploadCooldownTime:
					writer.String(vm_settings_[kUploadCooldownTime].c_str());
					writer.Uint(vm->UploadCooldownTime);
					break;
				case kUploadMaxSize:
					writer.String(vm_settings_[kUploadMaxSize].c_str());
					writer.Uint(vm->MaxUploadSize);
					break;
				case kUploadMaxFilename:
					writer.String(vm_settings_[kUploadMaxFilename].c_str());
					writer.Uint(vm->UploadMaxFilename);
					break;
				case kMOTD:
					writer.String(vm_settings_[kMOTD].c_str());
					writer.String(vm->MOTD.c_str());
					break;
			}
		}
		writer.EndObject();
	}
	// End array of VM settings
	writer.EndArray();
	// End "settings" object
	writer.EndObject();
}

void CollabVMServer::ParseServerSettings(rapidjson::Value& settings, rapidjson::Writer<rapidjson::StringBuffer>& writer) {
	// Copy the current config to a temporary one so that
	// changes are not made if a setting is invalid
	Config config = database_.Configuration;
	bool valid = true;
	for(auto it = settings.MemberBegin(); it != settings.MemberEnd(); it++) {
		for(size_t i = 0; i < sizeof(server_settings_) / sizeof(std::string); i++) {
			std::string key = std::string(it->name.GetString(), it->name.GetStringLength());
			if(key == server_settings_[i]) {
				rapidjson::Value& value = it->value;
				switch(i) {
					case kChatRateCount:
						if(value.IsUint()) {
							if(value.GetUint() <= std::numeric_limits<uint8_t>::max())
								config.ChatRateCount = value.GetUint();
							else {
								WriteJSONObject(writer, server_settings_[kChatRateCount], "Value too big");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, server_settings_[kChatRateCount], invalid_object_);
							valid = false;
						}
						break;
					case kChatRateTime:
						if(value.IsUint()) {
							if(value.GetUint() <= std::numeric_limits<uint8_t>::max())
								config.ChatRateTime = value.GetUint();
							else {
								WriteJSONObject(writer, server_settings_[kChatRateTime], "Value too big");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, server_settings_[kChatRateTime], invalid_object_);
							valid = false;
						}
						break;
					case kChatMuteTime:
						if(value.IsUint()) {
							if(value.GetUint() <= std::numeric_limits<uint8_t>::max())
								config.ChatMuteTime = value.GetUint();
							else {
								WriteJSONObject(writer, server_settings_[kChatMuteTime], "Value too big");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, server_settings_[kChatMuteTime], invalid_object_);
							valid = false;
						}
						break;
					case kMaxConnections:
						if(value.IsUint()) {
							if(value.GetUint() <= std::numeric_limits<uint8_t>::max()) {
								config.MaxConnections = value.GetUint();
							} else {
								WriteJSONObject(writer, server_settings_[kMaxConnections], "Value too big");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, server_settings_[kMaxConnections], invalid_object_);
							valid = false;
						}
						break;
					case kMaxUploadTime:
						if(value.IsUint()) {
							if(value.GetUint() <= std::numeric_limits<uint16_t>::max()) {
								config.MaxUploadTime = value.GetUint();
							} else {
								WriteJSONObject(writer, server_settings_[kMaxUploadTime], "Value too big");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, server_settings_[kMaxUploadTime], invalid_object_);
							valid = false;
						}
						break;
					case kBanCommand:
						if(value.IsString())
							config.BanCommand = std::string(value.GetString(), value.GetStringLength());
						else {
							WriteJSONObject(writer, server_settings_[kBanCommand], invalid_object_);
							valid = false;
						}
						break;
					case kJPEGQuality:
						if(value.IsUint()) {
							if(value.GetUint() <= 100 || value.GetUint() == 255) {
								config.JPEGQuality = value.GetUint();
							} else {
								WriteJSONObject(writer, server_settings_[kJPEGQuality], "Value too big");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, server_settings_[kJPEGQuality], invalid_object_);
							valid = false;
						}
						break;
					case kModEnabled:
						if(value.IsBool()) {
							config.ModEnabled = value.GetBool();
						} else {
							WriteJSONObject(writer, server_settings_[kModEnabled], invalid_object_);
							valid = false;
						}
						break;
					case kModPerms:
						if(value.IsUint()) {
							if(value.GetUint() <= std::numeric_limits<uint16_t>::max()) {
								config.ModPerms = value.GetUint();
							} else {
								WriteJSONObject(writer, server_settings_[kModPerms], "Value too big");
								valid = false;
							}
						} else {
							WriteJSONObject(writer, server_settings_[kModPerms], invalid_object_);
							valid = false;
						}
						break;
					case kBlacklistedUsernames:
						if(value.IsString()) {
							config.BlacklistedNames = std::string(value.GetString(), value.GetStringLength());

							// Refresh list of blacklisted usernames
							blacklisted_usernames_.clear();
							boost::split(blacklisted_usernames_, database_.Configuration.BlacklistedNames, boost::is_any_of(";"));
						} else {
							WriteJSONObject(writer, server_settings_[kBlacklistedUsernames], invalid_object_);
							valid = false;
						}
						break;
				}
				break;
			}
		}
	}

	// Only save the configuration if all settings valid
	if(valid) {
#ifdef USE_JPEG
		if(config.JPEGQuality <= 100)
			SetJPEGQuality(config.JPEGQuality);
#endif
		database_.Save(config);

		// Set the value of the "result" property to true to indicate success
		writer.Bool(true);
		// Append the updated settings to the JSON object
		WriteServerSettings(writer);

		std::cout << "Settings were updated" << std::endl;
	} else {
		std::cout << "Failed to update settings" << std::endl;
	}
}
