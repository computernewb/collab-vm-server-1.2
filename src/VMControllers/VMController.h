#pragma once
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>
#include <mutex>
#include <string>
#include <deque>
#include <stdint.h>
#include <memory>

#include "CollabVMUser.h"
#include "UploadInfo.h"
#include "GuacClient.h"
#include "UserList.h"
#include "Sockets/AgentClient.h"

class CollabVMServer;
struct VMSettings;

/**
 * A base class that is responsible for starting the hypervisor, running the VM,
 * creating a Guacamole client, and possibly a guest service controller. Derived
 * classes will implement functionality specific to the hypervisor.
 */
class VMController : public std::enable_shared_from_this<VMController>, public AgentCallback {
	friend GuacBroadcastSocket;

   public:

	virtual ~VMController() = default;

	virtual void ChangeSettings(const std::shared_ptr<VMSettings>& settings);

	/**
	 * Starts the hypervisor, runs the VM, and starts the Guacamole client.
	 */
	virtual void Start() = 0;

	enum StopReason {
		kNormal,  // The controller was stopped normally
		kRestart, // The controller should be restarted
		kRemove,  // The controller is being deleted
		kError	  // The controller failed to start
	};

	enum ControllerState {
		kStopped,
		kStarting,
		kRunning,
		kStopping
	};

	/**
	 * Stops the hypervisor and Guacamole client.
	 */
	virtual void Stop(StopReason reason);

	/**
	 * Disconnects all users and frees any resources used
	 * by the controller.
	 */
	virtual void CleanUp() = 0;

	/**
	 * Restores the VM to the specified snapshot.
	 */
	virtual void RestoreVMSnapshot() = 0;

	/**
	 * Send an ACPI poweroff signal to the VM.
	 */
	virtual void PowerOffVM() = 0;

	virtual void ResetVM() = 0;

	/**
	 * Callback for when the Guacamole client is initialized.
	 */
	virtual void OnGuacStarted() = 0;

	/**
	 * Callback for when the Guacamole client stops.
	 */
	virtual void OnGuacStopped() = 0;

	/**
	 * Callback for when the Guacamole client connects to the
	 * server.
	 */
	virtual void OnGuacConnect() = 0;

	/**
	* Callback for when the Guacamole client disconnects from the
	* server. If controller is not stopping, the client should
	* reconnect.
	*/
	virtual void OnGuacDisconnect(bool cleanup) = 0;

	const VMSettings& GetSettings() const {
		return *settings_;
	}

	virtual const std::string& GetErrorMessage() const = 0;

	virtual ControllerState GetState() const = 0;

	virtual StopReason GetStopReason() const = 0;

	void EndTurn(const std::shared_ptr<CollabVMUser>& user);

	void AddUser(const std::shared_ptr<CollabVMUser>& user);

	void RemoveUser(const std::shared_ptr<CollabVMUser>& user);

	void Vote(CollabVMUser& user, bool vote);

	void EndVote(bool cancelVote);

	void TurnRequest(const std::shared_ptr<CollabVMUser>& user, bool turnJack, bool isStaff);

	/**
	 * After a turn has ended, this function will update the turn
	 * queue and give the next user a turn.
	 */
	void NextTurn();

	std::shared_ptr<CollabVMUser> CurrentTurn() const {
		return current_turn_;
	}

	const std::deque<std::shared_ptr<CollabVMUser>>& GetTurnQueue() const {
		return turn_queue_;
	}

	void ClearTurnQueue();

	virtual bool IsRunning() const = 0;

	virtual void UpdateThumbnail() = 0;

	/**
	 * Called by the Guacamole client after a new thumbnail has
	 * been created.
	 */
	void NewThumbnail(std::string* str);

	inline std::string* GetThumbnail() const {
		return thumbnail_str_;
	}

	inline void SetThumbnail(std::string* str) {
		if(thumbnail_str_)
			delete thumbnail_str_;
		thumbnail_str_ = str;
	}

	//inline std::string& GetTurnListCache()
	//{
	//	return turn_list_cache_;
	//}

	//inline void SetTurnListCache(const std::string& str)
	//{
	//	turn_list_cache_ = str;
	//}

	inline UserList& GetUsersList() {
		return users_;
	}

	bool IsFileUploadValid(const std::shared_ptr<CollabVMUser>& user, const std::string& filename, size_t file_size, bool run_file);

	void UploadFile(const std::shared_ptr<UploadInfo>& info) {
		if(agent_)
			agent_->UploadFile(info);
	}

	// Only accessed by CollabVMServer from the processing thread
	std::string agent_os_name_;
	std::string agent_service_pack_;
	std::string agent_pc_name_;
	std::string agent_username_;
	uint32_t agent_max_filename_;
	bool agent_connected_;
	bool agent_upload_in_progress_;
	std::deque<std::shared_ptr<UploadInfo>> agent_upload_queue_;

   protected:
	VMController(CollabVMServer& server, boost::asio::io_service& service, const std::shared_ptr<VMSettings>& settings);

	virtual void OnAddUser(CollabVMUser& user) = 0;

	virtual void OnRemoveUser(CollabVMUser& user) = 0;

	void InitAgent(const VMSettings& settings, boost::asio::io_service& service);

	void OnAgentConnect(const std::string& os_name, const std::string& service_pack,
						const std::string& pc_name, const std::string& username, uint32_t max_filename) override;
	void OnAgentDisconnect(bool protocol_error) override;
	void OnAgentHeartbeatTimeout() override;
	void OnFileUploadStarted(const std::shared_ptr<UploadInfo>& info, std::string* filename) override;
	void OnFileUploadFailed(const std::shared_ptr<UploadInfo>& info /*, Reason*/) override;
	void OnFileUploadFinished(const std::shared_ptr<UploadInfo>& info) override;
	void OnFileUploadExecFinished(const std::shared_ptr<UploadInfo>& info, bool exec_success) override;

	CollabVMServer& server_;

	/**
	 * The io_service to use for guest service sockets and
	 * hypervisor controller sockets.
	 */
	boost::asio::io_service& io_service_;

	std::shared_ptr<VMSettings> settings_;

	StopReason stop_reason_;

	UserList users_;

	size_t connected_users_;

	/**
	 * Timer used to wait before attempting to reconnect to the agent socket.
	 */
	boost::asio::steady_timer agent_timer_;
	std::shared_ptr<AgentClient> agent_;
	std::string agent_address_;

   private:
	enum class VoteState {
		kIdle,
		kVoting,
		kCoolingdown
	};

	// Used for turn and vote timers. This means the max value for a
	// turn or a vote is about 9.1 hours.
	typedef std::chrono::duration<int32_t, std::milli> millisecs_t;

	void VoteEndedCallback(const boost::system::error_code& ec);

	void TurnTimerCallback(const boost::system::error_code& ec);

	boost::asio::steady_timer turn_timer_;

	std::deque<std::shared_ptr<CollabVMUser>> turn_queue_;
	std::shared_ptr<CollabVMUser> current_turn_;

	VoteState vote_state_;

	uint32_t vote_count_yes_;
	uint32_t vote_count_no_;

	/**
	 * This timer counts down the remaining time during a vote when
	 * vote_running_ is true or the time until another vote can be
	 * started when vote_running_ is false.
	 */
	boost::asio::steady_timer vote_timer_;

	std::string* thumbnail_str_;

	//std::string turn_list_cache_;

	const uint32_t kMaxFilenameLen = 100;
};
