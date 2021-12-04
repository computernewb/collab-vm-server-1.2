#pragma once
#include "VMController.h"
#include "Database/VMSettings.h"
#include "GuacVNCClient.h"
#include "Sockets/QMPClient.h"

// VMController already includes most of this..?
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>

#include <atomic>
#include <thread>
#include <mutex>
#include <vector>

class CollabVMServer;

/**
 * A VM controller which manages a QEMU virtual machine,
 * using the QMP protocol.
 */
class QEMUController : public VMController, public QMPCallback {
   public:
	enum ErrorCode {
		kQEMUError, // QEMU returned a nonzero status code after terminating
		kQMPFailed, // The QMP client failed to connect
		kVNCFailed	// The VNC client failed to connect
	};

	QEMUController(CollabVMServer& server, const std::shared_ptr<VMSettings>& settings);

	void ChangeSettings(const std::shared_ptr<VMSettings>& settings) override;

	~QEMUController() override;

	/**
	 * Start the QEMU process and wait for the socket to connect.
	 */
	void Start() override;

	/**
	 * Loads the snapshot by either sending the loadvm command to the monitor
	 * or stopping QEMU and restarting it with the loadvm command line argument
	 * depending on RestartForSnapshot.
	 */
	void RestoreVMSnapshot() override;

	void PowerOffVM() override;

	void ResetVM() override;

	void Stop(VMController::StopReason reason) override;

	void CleanUp() override;

	void OnGuacStarted() override;

	void OnGuacStopped() override;

	void OnGuacConnect() override;

	void OnGuacDisconnect(bool cleanup) override;

	void SendMonitorCommand(std::string cmd, QMPClient::ResultCallback resultCb);

	std::uint8_t GetKind() const override;

	/*
	 * Set to true when QEMU should be completely restarted
	 * whenever it needs to be resnapshotted.
	 */
	bool RestartForSnapshot;

	const std::string& GetErrorMessage() const override;

	bool IsRunning() const override {
		return internal_state_ != InternalState::kInactive;
	}

	ControllerState GetState() const override;

	StopReason GetStopReason() const override {
		return stop_reason_;
	}

	inline void UpdateThumbnail() override {
		guac_client_.UpdateThumbnail();
	}

	/**
	 * Callback for QMP state changes.
	 */
	void OnQMPStateChange(QMPClient::QMPState state) override;

   protected:
	void OnAddUser(CollabVMUser& user) override;

	void OnRemoveUser(CollabVMUser& user) override;

   private:
	/**
	* Sets the command used for starting QEMU. The comand should
	* not contain any of the following arguments:
	* -loadvm, -snapshot, -no-shutdown, -qmp, and -vnc.
	*/
	void SetCommand(const std::string& command);

	void InitQMP();

	/**
	 * Start the QEMU process using the specified command.
	 */
	void StartQEMU();

	/**
	 * Stop the QEMU process by either sending it a quit command
	 * via QMP or with a kill signal.
	 */
	void StopQEMU();

	/**
	 * Called after the QEMU process has been stopped.
	 */
	void OnQEMUStop();

	/**
	 * Terminates the QEMU process by sending the kill signal to it.
	 */
	void KillQEMU();

	void StartQMPCallback(const boost::system::error_code& ec);

	/**
	 * Wait one second and attempt to connect with QMP.
	 */
	void StartQMP();

	void StartGuacClientCallback(const boost::system::error_code& ec);

	void ProcessKillTimeout(const boost::system::error_code& ec);

	/**
	 * Wait one second and attempt to connect with the Guacamole client.
	 */
	void StartGuacClient();

	/**
	 * Called when the QEMU process has terminated.
	 */
	void OnChildExit();


	void GuacDisconnect();

	/**
	 * Sets the controller's state to inactive if the following conditions
	 * are met: the QEMU process terminated, the QMP client disconnected,
	 * and the Guacamole VNC client disconnected.
	 */
	void IsStopped();

	enum class InternalState {
		kInactive, // The controller hasn't been started
		//kQEMUStarting,	// The QEMU hypervisor is starting
		kQMPConnecting, // The QMP client is connecting
		kVNCConnecting, // The VNC client is connecting
		kConnected,		// The QEMU process is running, and the QMP and VNC clients are connected
		kStopping		// Waiting for QEMU process to terminate and VNC and QMP to close
	};

	
	void HandleChildSignal(const boost::system::error_code&, int signal);

	/**
	 * Splits a string into a vector.
	 * On Unix the wordexp function is used.
	 * On Windows the CommandLineToArgvW function is used.
	 */
	static std::vector<const char*> SplitCommandLine(const char* command);

	GuacVNCClient guac_client_;

	/**
	 * The current state of the controller.
	 */
	InternalState internal_state_;

	/**
	 * Indicates whether the QEMU process is running.
	 * If it is true, the pid_ member is valid.
	 */
	std::atomic<bool> qemu_running_;

	/**
	 * The QMP client used for controlling QEMU.
	 */
	std::shared_ptr<QMPClient> qmp_;

	std::string qmp_address_;

	/**
	 * The split command used to start QEMU.
	 */
	std::vector<const char*> qemu_command_;

	/**
	 * The name of the VM snapshot to restore when starting QEMU or restarting it.
	 * This only applies when the snapshot mode is set to VMSnapshot.
	 */
	// Currently unused
	std::string snapshot_;

	/**
	 * The number of times the client has failed to connect to
	 * either the QMP or VNC server. Once the count reaches the
	 * max number of attempts the client will stop trying and an
	 * error will occur.
	 */
	size_t retry_count_;

	ErrorCode error_code_;

#ifdef USE_SYSTEM_CLOCK
	typedef std::chrono::system_clock time_clock;
#else
	typedef std::chrono::steady_clock time_clock;
#endif

	/**
	 * This timer is used before attempting to connect to the QMP
	 * or VNC server or when waiting for the QEMU process to terminate.
	 */
	boost::asio::steady_timer timer_;

#ifndef _WIN32
	/**
	 * Process ID of QEMU. Only valid when state_ != kInactive.
	 */
	pid_t qemu_pid_;
	boost::asio::signal_set signal_;
#else
	/**
	 * Process information of QEMU on Windows. State message applies here.
	 */
	PROCESS_INFORMATION qemu_process_;
#endif
	
};
