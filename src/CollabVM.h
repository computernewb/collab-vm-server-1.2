#pragma once

#undef access
#include "Database/Database.h"

#include <string>
#include <cstring>
#include <memory>
#include <chrono>
#include <array>
#include <set>
#include <mutex>
#include <atomic>
#include <queue>
#include <cstdint>
#include <condition_variable>
#include <deque>
#include <list>
#include <random>

#include <websocketmm/fwd.h>

#include <boost/asio/steady_timer.hpp>

#include <rapidjson/writer.h>
#include "uriparser/Uri.h"

#include "VMControllers/VMController.h"
#include "VMControllers/QEMUController.h"
#include "Database/VMSettings.h"
#include "GuacUser.h"
#include "CollabVMUser.h"
#include "UploadInfo.h"

#include "Chat.h"

#ifdef _WIN32
	#define strncasecmp _strnicmp
	#define strcasecmp _stricmp
#else
	#include "StackTrace.hpp"
#endif

class GuacClient;
class GuacUser;

class CollabVMServer : public std::enable_shared_from_this<CollabVMServer> {
   public:
	explicit CollabVMServer(boost::asio::io_service& service);

	~CollabVMServer();

	/**
	 * Start the server.
	 * @param port The WebSocket server port to listen on.
	 * @param doc_root The path of the directory to expose via HTTP.
	 */
	void Run(uint16_t port, std::string doc_root);

	/**
	 * Stop listening for new clients and disconnect all existing ones.
	 */
	void Stop();

	void OnVMControllerStateChange(const std::shared_ptr<VMController>& controller, VMController::ControllerState state);

	/**
	 * Callback for when a VM controller becomes disconnected from
	 * a VM and it's resources need to be freed by the server to
	 * prevent race conditions.
	 */
	void OnVMControllerCleanUp(const std::shared_ptr<VMController>& controller);

	void OnVMControllerVoteEnded(const std::shared_ptr<VMController>& controller);

	/**
	 * Callback for when the turn queue changes for a VM controller.
	 * Calls the VMController::NextTurn function inside of the processing thread.
	 */
	void OnVMControllerTurnChange(const std::shared_ptr<VMController>& controller);

	void OnAgentConnect(const std::shared_ptr<VMController>& controller,
						const std::string& os_name, const std::string& service_pack,
						const std::string& pc_name, const std::string& username, uint32_t max_filename);
	void OnAgentDisconnect(const std::shared_ptr<VMController>& controller);
	//void OnAgentHeartbeatTimeout(const std::shared_ptr<VMController>& controller);
	void OnFileUploadFailed(const std::shared_ptr<VMController>& controller, const std::shared_ptr<UploadInfo>& info /*, Reason*/);
	void OnFileUploadFinished(const std::shared_ptr<VMController>& controller, const std::shared_ptr<UploadInfo>& info);
	void OnFileUploadExecFinished(const std::shared_ptr<VMController>& controller, const std::shared_ptr<UploadInfo>& info, bool exec_success);

	/**
	 * Updates the preview for the VM.
	 */
	void OnVMControllerThumbnailUpdate(const std::shared_ptr<VMController>& controller, std::string* str);

	/**
	 * Sends turn information to all user viewing a VM.
	 * @param current_user The user who has control of the VM (can be null).
	 * @param time_remaining The time remaining for the current user's turn.
	 */
	void BroadcastTurnInfo(VMController& controller, UserList& users, const std::deque<std::shared_ptr<CollabVMUser>>& turn_queue, CollabVMUser* current_turn, uint32_t time_remaining);

	/**
	 * Send turn info to the specified user because they just connected a VM.
	 */
	void SendTurnInfo(CollabVMUser& user, uint32_t time_remaining, const std::string& current_turn, const std::deque<std::shared_ptr<CollabVMUser>>& turn_queue);

	void BroadcastVoteInfo(const VMController& vm, UserList& users, bool vote_started, uint32_t time_remaining, uint32_t yes_votes, uint32_t no_votes);

	void SendVoteInfo(const VMController& vm, CollabVMUser& user, uint32_t time_remaining, uint32_t yes_votes, uint32_t no_votes);

	void UserStartedVote(const VMController& vm, UserList& users, CollabVMUser& user);

	void UserVoted(const VMController& vm, UserList& users, CollabVMUser& user, bool vote);

	void BroadcastVoteEnded(const VMController& vm, UserList& users, bool vote_succeeded);

	void VoteCoolingDown(CollabVMUser& user, uint32_t time_remaining);

	void ActionsChanged(VMController& controller, const VMSettings& settings) {
		SendActionInstructions(controller, settings);
	}

	/**
	 * Sends a message from a Guacamole client to WebSocket connection.
	 */
	void SendGuacMessage(std::weak_ptr<websocketmm::websocket_user> ptr, const std::string& str);

	void ExecuteCommandAsync(std::string command);
	void MuteUser(const std::shared_ptr<CollabVMUser>& user, bool permanent);
	void UnmuteUser(const std::shared_ptr<CollabVMUser>& user);

	// Shared definition of guacamole instruction garbage
#define GuacamoleInstruction(name) \
		void On##name##Instruction(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args);

	GuacamoleInstruction(Mouse)
	GuacamoleInstruction(Key)


	GuacamoleInstruction(Rename)
	GuacamoleInstruction(Connect)
	GuacamoleInstruction(Admin)
	GuacamoleInstruction(List)
	GuacamoleInstruction(Nop)
	GuacamoleInstruction(Chat)
	GuacamoleInstruction(Turn)
	GuacamoleInstruction(Vote)
	GuacamoleInstruction(File)

#undef GuacamoleInstruction

	/**
	 * Function pointer to a Guacamole instruction handler.
	 */
	typedef void (CollabVMServer::*GuacamoleInstruction)(const std::shared_ptr<CollabVMUser>& user, std::vector<char*>& args);


	void BroadcastMOTD(VMController& controller, const VMSettings& settings);

   private:

	/**
	 * Backwards-compatibility typedef. Oops!
	 */
	typedef websocketmm::server Server;

	// TODO: This should only be in .cpp, because
	// it is not needed anywhere else.

	enum class ActionType {
		kMessage,		   // Process message
		kAddConnection,	   // Add connection to map
		kRemoveConnection, // Remove connection from map
		kTurnChange,	   // Next turn
		kVoteEnded,		   // Vote ended
		kAgentConnect,	   // Agent connected
		kAgentDisconnect,  // Agent disconnected
		//kHttpUploadFinished,
		//kHttpUploadFailed,
		//kHttpUploadTimedout,
		kUploadEnded, // Agent upload ended
		//kHeartbeatTimedout,	// Heartbeat timed out
		kKeepAlive,		   // Broadcast keep-alive message
		kVMStateChange,	   // VM controller state changed
		kVMCleanUp,		   // Free a VM controller's resources
		kVMThumbnail,	   // Update a VM's thumbnail
		kUpdateThumbnails, // Update all VM thumbnails
		//kQEMU,			// kQEMU montior command result received
		kShutdown // Stop processing thread
	};

	/**
	 * Base class for actions that are consumed by the processing thread.
	 */
	struct Action {
		ActionType action;

		explicit Action(ActionType action)
			: action(action) {
		}

		// Define a default virtual destructor, otherwise when deleting we
		// will end up only calling the generated ~Action().
		// This is crucial, as destructors for derived classes will not be called,
		// leaving them in limbo (and even not deallocating the right size & such)
		//
		// We want all destructors to be called, including this one.
		virtual ~Action() = default;
	};

	/**
	 * Base class for actions which take in a user.
	 */
	struct UserAction : public Action {
		std::shared_ptr<CollabVMUser> user;

		UserAction(CollabVMUser& user, ActionType action)
			: Action(action),
			  user(user.shared_from_this()) {
		}
	};

	/**
	 * An action emitted when a WebSocket message is received.
	 */
	struct MessageAction : public UserAction {
		std::shared_ptr<const websocketmm::websocket_message> message;

		MessageAction(std::shared_ptr<const websocketmm::websocket_message>& msg, CollabVMUser& user, ActionType action)
			: UserAction(user, action),
			  message(msg) {
		}
	};

	struct VMAction : public Action {
		std::shared_ptr<VMController> controller;
		VMAction(const std::shared_ptr<VMController>& controller, ActionType action)
			: Action(action),
			  controller(controller) {
		}
	};

	struct VMStateChange : public VMAction {
		VMController::ControllerState state;
		VMStateChange(const std::shared_ptr<VMController>& controller, VMController::ControllerState state)
			: VMAction(controller, ActionType::kVMStateChange),
			  state(state) {
		}
	};

	struct VMThumbnailUpdate : public VMAction {
		std::string* thumbnail;
		VMThumbnailUpdate(const std::shared_ptr<VMController>& controller, std::string* thumbnail)
			: VMAction(controller, ActionType::kVMThumbnail),
			  thumbnail(thumbnail) {
		}
	};

	struct FileUploadAction : public Action {
		// why not just define the enum here?
		enum class UploadResult;

		FileUploadAction(const std::shared_ptr<UploadInfo>& upload_info, UploadResult result)
			: Action(ActionType::kUploadEnded),
			  upload_result(result),
			  upload_info(upload_info) {
		}

		enum class UploadResult {
			kFailed,
			kSuccess,
			kSuccessNoExec
		} upload_result;

		std::shared_ptr<UploadInfo> upload_info;
	};

	struct HttpAction : public Action {
		HttpAction(ActionType action, const std::shared_ptr<UploadInfo>& info)
			: Action(action),
			  upload_info(info) {
		}

		std::shared_ptr<UploadInfo> upload_info;
	};

	struct AgentConnectAction : public VMAction {
		std::string os_name;
		std::string service_pack;
		std::string pc_name;
		std::string username;
		uint32_t max_filename;
		AgentConnectAction(const std::shared_ptr<VMController>& controller,
						   const std::string& os_name, const std::string& service_pack,
						   const std::string& pc_name, const std::string& username, uint32_t max_filename)
			: VMAction(controller, ActionType::kAgentConnect),
			  os_name(os_name),
			  service_pack(service_pack),
			  pc_name(pc_name),
			  username(username),
			  max_filename(max_filename) {
		}
	};

	/**
	 * Boilerplate-condensing function to post an action into the processing queue.
	 * Arguments to this function are constructor arguments.
	 *
	 * \tparam TAction The action to post. Should be, or inherit from the Action class.
	 */
	template<class TAction, class ...Args>
	inline void PostAction(Args&&... args) {
		static_assert(std::is_base_of_v<Action, TAction> || std::is_same_v<TAction, Action>, "TAction needs to inherit from or be CollabVMServer::Action!");
		std::unique_lock<std::mutex> lock(process_queue_lock_);

		process_queue_.push(new TAction(std::forward<Args>(args)...));
		lock.unlock();
		process_wait_.notify_one();
	}

	struct case_insensitive_cmp {
		bool operator()(const std::string& str1, const std::string& str2) const {
			return strcasecmp(str1.c_str(), str2.c_str()) < 0;
		}
	};

	//bool ValidateSessionId(const std::string& cookies);

	bool OnValidate(std::weak_ptr<websocketmm::websocket_user> handle);
	void OnOpen(std::weak_ptr<websocketmm::websocket_user> handle);
	void OnClose(std::weak_ptr<websocketmm::websocket_user> handle);

	void TimerCallback(const boost::system::error_code& ec, ActionType action);
	void VMPreviewTimerCallback(const boost::system::error_code ec);
	void IPDataTimerCallback(const boost::system::error_code& ec);

	/**
	 * Determines whether an IPData object should be deleted.
	 */
	bool ShouldCleanUpIPData(IPData& ip_data) const;

	void RemoveConnection(std::shared_ptr<CollabVMUser>& user);

	void UpdateVMStatus(const std::string& vm_name, VMController::ControllerState state);

	/**
	 * Callback for when the result from a QEMU monitor command is
	 * received.
	 */
	void OnQEMUResponse(std::weak_ptr<CollabVMUser> data, rapidjson::Document& d);

	//std::string GenerateUuid();

	void OnMessageFromWS(std::weak_ptr<websocketmm::websocket_user> handle, std::shared_ptr<const websocketmm::websocket_message> msg);
	void SendWSMessage(CollabVMUser& user, const std::string& str);

	/**
	 * The main loop for the processing thread.
	 */
	void ProcessingThread();

	void AppendChatMessage(std::ostringstream& ss, ChatMessage* chat_msg);

	/**
	 * Sends the remembered chat history to the specified user.
	 */
	void SendChatHistory(CollabVMUser& user);

	/**
	 * Checks whether or not a username is valid.
	 * For the username to be valid it must contain only numbers, letters,
	 * spaces, dashes, underscores, and dots, and it must not begin or end
	 * with a space. More than one consecutive space is not allowed.
	 */
	bool ValidateUsername(const std::string& username);

	enum UsernameChangeResult : char {
		kSuccess = '0',		  // The username was successfully changed
		kUsernameTaken = '1', // Someone already has the username or it is registered to an account
		kInvalid = '2',		  // The requested username had invalid characters
		kBlacklisted = '3'
	};

	/**
	 * Sends the result of a username change to the specified client and
	 * if the result was successful a rename instruction will be broadcasted to
	 * all other online users. If the result successful and the client did not
	 * have an old username, a list of all the online users will be sent to 
	 * the client.
	 *
	 * @param data The client to Send the response to
	 * @param newUsername The new username for the client
	 * @param result The result of the username change
	 * @param send_history Whether the chat history should be sent. Only valid
	 * if the user did not previously have a username.
	 */
	void ChangeUsername(const std::shared_ptr<CollabVMUser>& data, const std::string& new_username, UsernameChangeResult result, bool send_history);

	/**
	 * The the list of all online users to the specified client.
	 */
	void SendOnlineUsersList(CollabVMUser& user);

	/**
	 * Sends an action instruction to all users currently connected to the VMController.
	 */
	void SendActionInstructions(VMController& controller, const VMSettings& settings);


	bool IsFilenameValid(const std::string& filename);


	void OnUploadTimeout(const boost::system::error_code ec, std::shared_ptr<UploadInfo> upload_info);
	void StartFileUpload(CollabVMUser& user);
	void SendUploadResultToIP(IPData& ip_data, const CollabVMUser& user, const std::string& instr);

	enum class FileUploadResult {
		kUserDisconnected,
		kHttpUploadTimedOut,
		kHttpUploadFailed,
		kAgentUploadSucceeded,
		kAgentUploadFailed
	};

	/**
	 * Deletes the temporary file used for the upload, sets the cooldown time for the IPData,
	 * and sends a success or fail message to the user.
	 */
	void FileUploadEnded(const std::shared_ptr<UploadInfo>& upload_info, const std::shared_ptr<CollabVMUser>* user, FileUploadResult result);

	void StartNextUpload();

	void CancelFileUpload(CollabVMUser& user);

	void SetUploadCooldownTime(IPData& ip_data, uint32_t time);

	bool SendUploadCooldownTime(CollabVMUser& user, const VMController& vm_controller);

	void BroadcastUploadedFileInfo(UploadInfo& upload_info, VMController& vm_controller);

	/**
	 * Generate a random username for a user who does not have one.
	 */
	std::string GenerateUsername();

	/**
	 * Escapes HTML characters in a string to prevent XSS.
	 */
	std::string EncodeHTMLString(const char* str, size_t strLen);

	/**
	 * Execute a function from the admin panel.
	 */
	std::string PerformConfigFunction(const std::string& json);

	/**
	 * Create a JSON string containing the server's current
	 * settings and each VM's settings.
	 */
	std::string GetServerSettings();

	/**
	 * Append the server's current settings and each VM's settings
	 * and to the JSON object.
	 */
	void WriteServerSettings(rapidjson::Writer<rapidjson::StringBuffer>& writer);

	/**
	 * Parse server settings and update the database config.
	 */
	void ParseServerSettings(rapidjson::Value& settings, rapidjson::Writer<rapidjson::StringBuffer>& writer);

	bool ParseVMSettings(VMSettings& vm, rapidjson::Value& settings, rapidjson::Writer<rapidjson::StringBuffer>& writer);

	std::shared_ptr<VMController> CreateVMController(const std::shared_ptr<VMSettings>& vm);

	bool FindIPv4Data(const boost::asio::ip::address_v4& addr, IPData*& ip_data);
	bool FindIPData(const boost::asio::ip::address& addr, IPData*& ip_data);
	IPData* CreateIPv4Data(const boost::asio::ip::address_v4& addr, bool one_connection);
	IPData* CreateIPData(const boost::asio::ip::address& addr, bool one_connection);
	void DeleteIPData(IPData& ip_data);

	boost::asio::io_service& service_;
	std::shared_ptr<Server> server_;

	CollabVM::Database database_;

	bool stopping_;

	/**
	 * Maps WebSocket++ handles to CollabVMUser objects.
	 * Each connection will have it's own CollabVMUser structure
	 * associated with it. This map should only be modified from within
	 * the processing thread. If the map needs to be accessed from another
	 * thread then it must acquire the _connectionsLock mutex first.
	 */
	std::set<std::shared_ptr<CollabVMUser>, std::owner_less<std::shared_ptr<CollabVMUser>>> connections_;

	/**
	 * Maps usernames to CollabVMUser objects.
	 * Modifying and accessing this map should treated the same as
	 * as _connections map above.
	 */
	std::map<std::string, std::shared_ptr<CollabVMUser>, case_insensitive_cmp> usernames_;

	/**
	 * List of usernames that should not be allowed
	 */
	std::vector<std::string> blacklisted_usernames_;

	std::set<std::shared_ptr<CollabVMUser>, std::owner_less<std::shared_ptr<CollabVMUser>>> admin_connections_;

	std::map<uint32_t, IPData*> ipv4_data_;
	std::map<std::array<uint8_t, 16>, IPData*> ipv6_data_;
	std::mutex ip_lock_;

	/**
	 * A queue containing actions for the processing thread to perform.
	 */
	std::queue<Action*> process_queue_;
	std::mutex process_queue_lock_;
	std::condition_variable process_wait_;

	/**
	 * A timer that sends keep-alive instructions to all the websocket clients.
	 */
	boost::asio::steady_timer keep_alive_timer_;

	/**
	 * Refreshes the thumbnails displayed for each running VM.
	 */
	boost::asio::steady_timer vm_preview_timer_;

	/**
	 * The frequency that VMs will update their thumbnails.
	 */
	const uint16_t kVMPreviewInterval = 5;

	/**
	 * A timer that cleans up IP data that is no longer needed.
	 */
	boost::asio::steady_timer ip_data_timer;

	bool ip_data_timer_running_;

	/**
	 * The frequency in minutes that the IP data timer will be called.
	 */
	static const uint8_t kIPDataTimerInterval;

	/**
	 * How frequently keep-alive instructions should be sent to each client.
	 * Measured in seconds.
	 */
	const uint8_t kKeepAliveInterval = 5;

	/**
	 * How long before a client is disconnected.
	 * Measured in seconds.
	 */
	const uint8_t kKeepAliveTimeout = 15;

	std::string doc_root_;

	/**
	 * A map containing a controller for each VM that is running.
	 * Each controller has a unique ID that clients will use to refer to it.
	 */
	std::map<std::string, std::shared_ptr<VMController>> vm_controllers_;

	std::string admin_session_id_;

	std::thread process_thread_;
	std::atomic<bool> process_thread_running_;

	/**
	 * Prevents the io_service from exiting before all the VM controllers
	 * are stopped.
	 */
	//std::unique_ptr<boost::asio::io_service::work> asio_work_;

	/**
	 * The RNG used for generating guest usernames.
	 */
	std::uniform_int_distribution<uint32_t> guest_rng_;
	std::default_random_engine rng_;

	/**
	 * Circular buffer used for storing chat history.
	 */
	ChatMessage* chat_history_;

	/**
	 * Begin index for the circular chat history buffer.
	 */
	uint8_t chat_history_begin_;

	/**
	 * End index for the circular chat history buffer.
	 */
	uint8_t chat_history_end_;

	/**
	 * The number of messages in the chat history buffer.
	 */
	uint8_t chat_history_count_;

	const size_t kMaxChatMsgLen = 100;

	const size_t kMinUsernameLen = 3;
	const size_t kMaxUsernameLen = 20;

	/**
	 * Maximum number of login attempts before the IP is blocked.
	 */
	static const size_t kMaxLoginTries = 3;

	/**
	 * The amount of time in minutes an IP address is blocked after failing
	 * to login.
	 */
	static const size_t kLoginIPBlockTime = 60;

	/**
	 * The maximum number of file uploads that can occur simultaneously.
	 */
	const size_t kMaxFileUploads = 10;

	/**
	 * The amount of time a user has to begin an upload in seconds.
	 */
	const size_t kUploadStartTimeout = 10;

	/**
	 * The current number of file uploads in progress.
	 */
	size_t upload_count_;

	const size_t kMaxChunkSize = 4096;

	/**
	 * The max string length of a base64 encoded file chunk.
	 */
	const size_t kMaxBase64ChunkSize = ((kMaxChunkSize + 2) / 3) * 4;

	/**
	 * After the maximum number of simultaneous uploads has been reached
	 * users will be added to this queue to wait until their file upload can begin.
	 */
	std::deque<std::shared_ptr<CollabVMUser>> upload_queue_;

	const std::string kFileUploadPath = "uploads/";

	std::mutex upload_lock_;
	std::map<std::string, std::shared_ptr<UploadInfo>, case_insensitive_cmp> upload_ids_;
};
