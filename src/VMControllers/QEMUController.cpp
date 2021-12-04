#include "VMControllers/QEMUController.h"
#include "CollabVM.h"
#ifdef _WIN32
	#include <Windows.h>
	#include <shellapi.h>
#else
	#include <unistd.h>
	#include <errno.h>
	#include <sys/types.h>
	#include <sys/wait.h>
	#include <wordexp.h>
	#include <system_error>
	#include <signal.h>

	#include <dirent.h>
	#include <sys/resource.h>

	#ifdef __linux__
		#include <sys/prctl.h>
	#endif
#endif
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <cstdio>
#include <string>
#include <iostream>
#include <functional>
#include <memory>
#include <sstream>

static const std::string kErrorMessages[] = {
	"Failed to start QEMU",
	"Failed to connect to QMP",
	"Failed to connect to VNC"
};

/**
 * A single thread is created for all QMPClient instances
 * to use.
 */
static std::mutex qmp_thread_mutex_;
static int qmp_count_ = 0;
static std::thread qmp_thread_;
static boost::asio::io_service::work* qmp_work_;
static boost::asio::io_service* qmp_service_;

#ifndef _WIN32
/**
 * Gets all tasks for a given PID, using Linux /proc filesystem
 */
std::vector<pid_t> GetProcessTasks(pid_t process) {
	DIR* tasks = nullptr;
	char buf[40];
	snprintf(&buf[0], sizeof(buf), "/proc/%d/tasks", process);
	tasks = opendir(&buf[0]);

	if(tasks) {
		std::vector<pid_t> ret;

		struct dirent* entry;
		while((entry = readdir(tasks)) != nullptr) {
			// Catch . and .. immediately
			if(entry->d_name[0] == '.')
				continue;

			char* end = nullptr;
			auto tid = static_cast<pid_t>(strtol(entry->d_name, &end, 10));

			// If strtol() failed on the string (it set end to the start pointer)
			// continue on.
			if(end == &entry->d_name[0])
				continue;

			ret.push_back(tid);
		}

		closedir(tasks);
		return ret;
	} else {
		// fail, so let's just return a vector with just the process "task"
		return { process };
	}
}

/**
 * Renice a task.
 */
void ReniceTask(pid_t pid, int nice) {
	// only set the nice level if we *need* to
	if(getpriority(PRIO_PROCESS, pid) != nice) {
		std::cout << "[QEMU] Setting task " << pid << " nice level to " << nice << "\n";
		if(setpriority(PRIO_PROCESS, pid, nice) == -1) {
			std::cout << "[QEMU] setpriority(PRIO_PROCESS, " << pid << ", " << nice << ") returned -1..?\n";
		}
	}
}

/**
 * Helper to renice all tasks of a process
 */
void ReniceAllTasks(pid_t process, int nice) {
	for(auto pid : GetProcessTasks(process))
		ReniceTask(pid, nice);
}
#endif

QEMUController::QEMUController(CollabVMServer& server, const std::shared_ptr<VMSettings>& settings)
	: VMController(server, settings),
	  guac_client_(server, *this, users_, settings->VNCAddress, settings->VNCPort),
	  internal_state_(InternalState::kInactive),
	  qemu_running_(false),
	  timer_(server.GetIOContext()),
	  retry_count_(0)
#ifndef _WIN32
	  ,
	  signal_(server.GetIOContext(), SIGCHLD)
#endif
{
	SetCommand(settings->QEMUCmd);

	std::lock_guard<std::mutex> lock(qmp_thread_mutex_);
	if(!qmp_count_++) {
		qmp_service_ = new boost::asio::io_service();
		qmp_work_ = new boost::asio::io_service::work(*qmp_service_);
		qmp_thread_ = std::thread([](boost::asio::io_service* service) {
			service->run();
			delete service;
		},
								  qmp_service_);
		qmp_thread_.detach();
	}

	InitQMP();
}

void QEMUController::ChangeSettings(const std::shared_ptr<VMSettings>& settings) {
	bool restart = false;
	if(settings->QEMUCmd != settings_->QEMUCmd) {
		SetCommand(settings_->QEMUCmd);
		restart = true;
	}
	if(settings->VNCAddress != settings_->VNCAddress || settings->VNCPort != settings_->VNCPort) {
		guac_client_.SetEndpoint(settings_->VNCAddress, settings_->VNCPort);
		restart = true;
	}
	if(settings->QMPAddress != settings_->QMPAddress || settings->QMPPort != settings_->QMPPort) {
		//qmp_->SetEndpoint(settings_->QMPAddress, settings_->QMPPort);
		restart = true;
	}
	if(settings->QMPSocketType != settings_->QMPSocketType) {
		// TODO:
		// InitQMP();
		restart = true;
	}
	
	VMController::ChangeSettings(settings);
	settings_ = settings;
	if(restart)
		Stop(StopReason::kRestart);
}

QEMUController::~QEMUController() {
	std::lock_guard<std::mutex> lock(qmp_thread_mutex_);
	if(!--qmp_count_)
		delete qmp_work_;
}

#ifndef _WIN32
void QEMUController::HandleChildSignal(const boost::system::error_code& ec, int signal) {
	if(ec)
		return;
	int status = 0;
	pid_t pid;
	while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		// Check if the process exited
		if(WIFEXITED(status)) {
			qemu_running_ = false;
			std::cout << "QEMU child process with PID: " << pid << " has terminated with status: " << WEXITSTATUS(status) << std::endl;

			// Stop the timer
			boost::system::error_code ec;
			timer_.cancel(ec);

			// If we were expecting QEMU to stop
			if(internal_state_ == InternalState::kStopping) {
				IsStopped();
			} else if(internal_state_ != InternalState::kInactive) {
				// If QEMU's exit code is not zero and
				// QMP is not currently connected it probably means
				// that is was never able to connect because QEMU was
				// started with invalid arguments
				if(WEXITSTATUS(status) != 0 /* && !qmp.IsConnected()*/) {
					internal_state_ = InternalState::kStopping;
					error_code_ = ErrorCode::kQEMUError;
					Stop(StopReason::kError);

					IsStopped();

					std::cout << "QEMU terminated with a non-zero status code which indicates an error. "
								 "Check the command for any invalid arguments."
							  << std::endl;
				} else {
					// Restart QEMU
					StartQEMU();
				}
			}
		}
	}
	if(qemu_running_ && internal_state_ != InternalState::kStopping && internal_state_ != InternalState::kInactive)
		signal_.async_wait(std::bind(&QEMUController::HandleChildSignal,
									 std::static_pointer_cast<QEMUController>(shared_from_this()), std::placeholders::_1, std::placeholders::_2));
}
#endif

void QEMUController::Start() {
	if(internal_state_ != InternalState::kInactive)
		return;

	server_.OnVMControllerStateChange(shared_from_this(), VMController::ControllerState::kStarting);

	std::weak_ptr<QEMUController> con(std::static_pointer_cast<QEMUController>(shared_from_this()));
	// The STOP event will be received after the SHUTDOWN
	// event when the -no-shutdown argument is provided
	qmp_->RegisterEventCallback(QMPClient::Events::STOP,
								[con](rapidjson::Document& d) {
									auto ptr = con.lock();
									if(!ptr)
										return;

									std::cout << "[QEMU] Stop event occurred" << std::endl;

									if(ptr->internal_state_ != InternalState::kStopping) {
										if(ptr->settings_->RestoreOnShutdown &&
										   ptr->settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kHDSnapshots /* ||
				(ptr->settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kVMSnapshots && ptr->RestartForSnapshot)*/
										) {
											// Restart QEMU to restore the snapshot
											std::cout << "[QEMU] Restarting QEMU..." << std::endl;

											ptr->StopQEMU();
										} else {
											// Reset QEMU to reboot the VM
											std::cout << "[QEMU] Resetting QEMU..." << std::endl;

											// If the reset event doesn't occur within five seconds, kill the process
											// This is a workaround for when communication with QMP has been
											// lost but the socket is still connected
											boost::system::error_code ec;
											ptr->timer_.expires_from_now(std::chrono::seconds(5), ec);
											ptr->timer_.async_wait(std::bind(&QEMUController::ProcessKillTimeout,
																			 std::static_pointer_cast<QEMUController>(ptr), std::placeholders::_1));

											ptr->qmp_->SystemReset();
										}
									}
								});

	qmp_->RegisterEventCallback(QMPClient::Events::RESET,
								[con](rapidjson::Document& d) {
									std::cout << "[QEMU] Reset event occurred" << std::endl;

									auto ptr = con.lock();
									if(!ptr)
										return;

									// Cancel the timeout timer
									boost::system::error_code ec;
									ptr->timer_.cancel(ec);

									// After QEMU has been reset, load the snapshot
									if(ptr->settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kVMSnapshots &&
									   !ptr->snapshot_.empty()) {
										if(ptr->RestartForSnapshot) {
											// This shouldn't happen
											// The QEMU process must be restarted
											// Restart QEMU to restore the snapshot
											std::cout << "[QEMU] Restarting QEMU..." << std::endl;

											ptr->StopQEMU();
										} else {
											// Send the loadvm command to the monitor to restore the snapshot
											ptr->qmp_->SendMonitorCommand("loadvm " + ptr->snapshot_,
																		  [con](rapidjson::Document&) {
																			  if(auto ptr = con.lock()) {
																				  std::cout << "Received result for loadvm command" << std::endl;
																				  // Send the continue command to resume execution
																				  ptr->qmp_->SystemResume();
																			  }
																		  });
										}
									} else if(ptr->settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kHDSnapshots)
										ptr->qmp_->SystemResume();
								});

#ifdef _WIN32

#else
	signal_.async_wait(bind(&QEMUController::HandleChildSignal,
							std::static_pointer_cast<QEMUController>(shared_from_this()), std::placeholders::_1, std::placeholders::_2));
#endif

	StartQEMU();
}

void QEMUController::SetCommand(const std::string& command) {
	// Free all arguments
	
	//for(auto & it : qemu_command_)
	//	delete[] it;

	qemu_command_.clear();

	if(!command.empty())
		qemu_command_ = SplitCommandLine(command.c_str());
}

void QEMUController::InitQMP() {
#ifdef _WIN32
	qmp_address_ = settings_->QMPAddress;
	std::string port = std::to_string(settings_->QMPPort);
	qmp_address_.reserve(qmp_address_.length() + 1 + port.length());
	qmp_address_ += ':';
	qmp_address_ += port;
	qmp_ = std::make_shared<QMPTCPClient>(*qmp_service_, settings_->QMPAddress, settings_->QMPPort);
#else
	if(settings_->QMPSocketType == VMSettings::SocketType::kTCP) {
		qmp_address_ = settings_->QMPAddress;
		std::string port = std::to_string(settings_->QMPPort);
		qmp_address_.reserve(qmp_address_.length() + 1 + port.length());
		qmp_address_ += ':';
		qmp_address_ += port;
		qmp_ = std::make_shared<QMPTCPClient>(*qmp_service_, settings_->QMPAddress, settings_->QMPPort);
	} else if(settings_->QMPSocketType == VMSettings::SocketType::kLocal) {
		if(settings_->QMPAddress.empty()) {
	#ifdef _WIN32
			qmp_address_ = ":5800";
	#else
			// Unix domain sockets need to have a valid file path
			qmp_address_ = P_tmpdir "/collab-vm-qmp-";
	#endif
			qmp_address_ += settings_->Name;
		} else
			qmp_address_ = settings_->QMPAddress;


		qmp_ = std::make_shared<QMPLocalClient>(*qmp_service_, qmp_address_);
	}
#endif
}

void QEMUController::RestoreVMSnapshot() {
	if(internal_state_ != InternalState::kConnected && internal_state_ != InternalState::kVNCConnecting)
		return;

	if(settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kVMSnapshots && !snapshot_.empty()) {
		if(RestartForSnapshot)
			StopQEMU();
		else
			qmp_->LoadSnapshot(snapshot_, QMPClient::ResultCallback());
	} else if(settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kHDSnapshots)
		StopQEMU();
	else
		qmp_->SystemStop();
}

void QEMUController::PowerOffVM() {
	if(qmp_->IsConnected())
		qmp_->SystemPowerDown();
}

void QEMUController::ResetVM() {
	if(qmp_->IsConnected())
		qmp_->SystemReset();
}

void QEMUController::StartQEMU() {
#ifdef _WIN32

	if(settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kVMSnapshots && !snapshot_.empty()) {
		// Append loadvm command to start with snapshot
		qemu_command_.push_back("-loadvm");
		qemu_command_.push_back(snapshot_.c_str());
	} else if(settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kHDSnapshots)
		qemu_command_.push_back("-snapshot");

	//if (settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::)
	qemu_command_.push_back("-no-shutdown");

	// QMP address
	qemu_command_.push_back("-qmp");
	std::string qmp_arg;

	qmp_arg = "tcp:";
	qmp_arg += qmp_address_;
	qmp_arg += ",server,nodelay";
	qemu_command_.push_back(qmp_arg.c_str());

	/*
	else
	{
		qmp_arg = "pipe:";
		qmp_arg += qmp_address_;
		qmp_arg += ",server";
		qemu_command_.push_back(qmp_arg.c_str());
	}
	*/

	// Append VNC argument
	qemu_command_.push_back("-vnc");
	// Subtract 5900 from the port number and append it to the hostname
	std::string vnc_arg = settings_->VNCAddress + ':' + std::to_string(settings_->VNCPort - 5900);
	qemu_command_.push_back(vnc_arg.c_str());

	std::cout << "Starting QEMU with command:\n";

	std::string qemu_cmdline;

	for(auto it = qemu_command_.begin(); it != qemu_command_.end(); it++) {
		std::cout << *it << ' ';
		qemu_cmdline += std::string(*it);
		qemu_cmdline += " ";
	}
	std::cout << "\n";

	STARTUPINFO si;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&qemu_process_, sizeof(qemu_process_));

	char* QemuCmdLineMutable = (char*)calloc(qemu_cmdline.length() + 1, sizeof(char)); // more idomatic
	strncpy(QemuCmdLineMutable, qemu_cmdline.c_str(), qemu_cmdline.length());

	BOOL ProcessCreateStatus = CreateProcess(NULL, QemuCmdLineMutable, NULL, NULL, FALSE, 0, NULL, NULL, &si, &qemu_process_);

	std::cout << "QEMU PID: " << qemu_process_.dwProcessId << std::endl;

	// free the mutable buffer to avoid a memleak
	free((char*)QemuCmdLineMutable);
#else

	// pid of the child before forking
	pid_t parent_before_fork = getpid();
	pid_t pId = fork();

	if(pId == 0) {
		// TODO: Should a new process group or session be created for QEMU?
		// Creating a new process group causes QEMU to freeze when the -nographic
		// argument is specified
		// Change process group
		/*if (setpgid(0, 0))
			std::cout << "setpgid failed. errorno: " << errno << std::endl;*/


#ifdef __linux__
		// If the collab-vm-server dies, we need to be terminated as well
		// so the server can restart alright.
		int prctl_result = prctl(PR_SET_PDEATHSIG, SIGTERM);
		if (prctl_result == -1) {
			perror(0);
			exit(1);
		}
#endif

		// Test in case the original parent exited just before the prctl() call. 
		// If it did, then we exit on our own accord.
		if (getppid() != parent_before_fork)
			exit(1);

		// Close all other FD's other than stdin,
		// stdout, and stderr, avoiding fd-leak scenarios.
		// 
		// This is something cosmic (possibly) considered, but never actually did.
		// Alas, I had to end up doing it years later, to add to the duct
		// tape flavoring of this server.
		//
		{
			const int fd_limit = static_cast<int>(sysconf(_SC_OPEN_MAX));
			for(int i = STDERR_FILENO + 1; i < fd_limit; ++i)
				close(i);
		}

		// The qemu_command_ vector is only modified inside of the child process
		if(settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kVMSnapshots && !snapshot_.empty()) {
			// Append loadvm command to start with snapshot
			qemu_command_.push_back("-loadvm");
			qemu_command_.push_back(snapshot_.c_str());
		} else if(settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kHDSnapshots)
			qemu_command_.push_back("-snapshot");

		//if (settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::)
		qemu_command_.push_back("-no-shutdown");

		// QMP address
		qemu_command_.push_back("-qmp");
		std::string qmp_arg;
		if(settings_->QMPSocketType == VMSettings::SocketType::kTCP) {
			qmp_arg = "tcp:";
			qmp_arg += qmp_address_;
			qmp_arg += ",server,nodelay";
			qemu_command_.push_back(qmp_arg.c_str());
		} else {
			qmp_arg = "unix:";
			qmp_arg += qmp_address_;
			qmp_arg += ",server";
			qemu_command_.push_back(qmp_arg.c_str());
		}

		if(access(qmp_address_.c_str(), F_OK) == 0) {
			std::cout << "[QEMU] Deleting old " << qmp_address_ << " socket so the VM will work\n";
			unlink(qmp_address_.c_str());
		}

		// Append VNC argument
		qemu_command_.push_back("-vnc");
		// Subtract 5900 from the port number and append it to the hostname
		std::string vnc_arg = settings_->VNCAddress + ':' + std::to_string(settings_->VNCPort - 5900);
		qemu_command_.push_back(vnc_arg.c_str());

		// Null terminate the arguments list
		qemu_command_.push_back(nullptr);
		std::cout << "Starting QEMU with command:\n";

		for(auto & it : qemu_command_) {
			if(it != nullptr)
				std::cout << it << ' ';
		}
		std::cout << std::endl;

		/*if (redirect_fd(STDIN_FILENO, O_RDONLY)
			|| redirect_fd(STDOUT_FILENO, O_WRONLY)
			|| redirect_fd(STDERR_FILENO, O_WRONLY))
		{
			std::cout << "Failed to redirect standard output for QEMU child process" << std::endl;
		}*/

		exit(execvp(qemu_command_[0], (char* const*)qemu_command_.data()));
	} else if(pId < 0) {
		// Failed to fork
		throw std::system_error(errno, std::system_category(), "fork() failed when trying to start QEMU");
	}

	std::cout << "QEMU process ID: " << pId << std::endl;
	qemu_pid_ = pId;
#endif
	qemu_running_ = true;
	internal_state_ = InternalState::kQMPConnecting;
	retry_count_ = 0;
	StartQMP();
}

void QEMUController::StopQEMU() {
	// Attempt to stop QEMU with QMP if it's connected
	if(qmp_->IsConnected()) {
		KillQEMU();
	} else {
		KillQEMU();
	}
}

void QEMUController::OnQEMUStop() {
	if(internal_state_ == InternalState::kStopping) {
		IsStopped();
	} else {
		// Restart QEMU
		StartQEMU();
	}
}

void QEMUController::KillQEMU() {
#ifndef _WIN32
	::kill(qemu_pid_, SIGKILL);
#else
	// Open a handle to the QEMU process, then terminate it hard.
	HANDLE hQemuProcess = OpenProcess(PROCESS_TERMINATE, FALSE, qemu_process_.dwProcessId);
	TerminateProcess(hQemuProcess, 9);
#endif
	qemu_running_ = false;
	OnQEMUStop();
}

void QEMUController::StartQMPCallback(const boost::system::error_code& ec) {
	if(!ec && internal_state_ == InternalState::kQMPConnecting)
		qmp_->Connect(std::weak_ptr<QEMUController>(std::static_pointer_cast<QEMUController>(shared_from_this())));
}

void QEMUController::StartQMP() {
	// Wait one second before attempting to connect to QEMU's QMP server
	boost::system::error_code ec;
	timer_.expires_from_now(std::chrono::seconds(1), ec);
	// TODO:
	// if (ec) ...
	timer_.async_wait(std::bind(&QEMUController::StartQMPCallback,
								std::static_pointer_cast<QEMUController>(shared_from_this()), std::placeholders::_1));
}

void QEMUController::StartGuacClientCallback(const boost::system::error_code& ec) {
	if(!ec && internal_state_ == InternalState::kVNCConnecting)
		guac_client_.Start();
}

void QEMUController::StartGuacClient() {
	boost::system::error_code ec;
	timer_.expires_from_now(std::chrono::seconds(1), ec);
	// TODO:
	// if (ec) ...
	timer_.async_wait(std::bind(&QEMUController::StartGuacClientCallback,
								std::static_pointer_cast<QEMUController>(shared_from_this()), std::placeholders::_1));
}

void QEMUController::ProcessKillTimeout(const boost::system::error_code& ec) {
	if(ec)
		return;
	std::cout << "QEMU did not terminate within 5 seconds. Killing process..." << std::endl;
#ifndef _WIN32
	boost::system::error_code err;
	signal_.cancel(err);
	// TODO:
	// if (err) ...
#endif
	KillQEMU();
}

void QEMUController::Stop(VMController::StopReason reason) {
	if(internal_state_ == InternalState::kInactive || internal_state_ == InternalState::kStopping)
		return;

	internal_state_ = InternalState::kStopping;
	stop_reason_ = reason;

	VMController::Stop(reason);

	// Stop the timer
	boost::system::error_code ec;
	timer_.cancel(ec);

	// Stop the Guacamole client
	guac_client_.Stop();

	StopQEMU();
}

void QEMUController::IsStopped() {
	if(guac_client_.GetState() == GuacClient::ClientState::kStopped &&
	   !qmp_->IsConnected() && !qemu_running_) {
		internal_state_ = InternalState::kInactive;

		//server_.OnVMControllerStop(shared_from_this(), stop_reason_);
		server_.OnVMControllerStateChange(shared_from_this(), VMController::ControllerState::kStopped);
	}
}

void QEMUController::GuacDisconnect() {
	if(!qmp_->IsConnected())
		return;

	// Restart the Guacamole client if we are not stopping
	if(internal_state_ == InternalState::kVNCConnecting) {
		std::cout << "Gaucamole client failed to connect.";
		// If we have exceeded the max number of connection attempts
		if(++retry_count_ >= settings_->MaxAttempts) {
			std::cout << "Max number attempts has been exceeded. Stopping...";

			error_code_ = ErrorCode::kVNCFailed;
			Stop(StopReason::kError);
		} else {
			std::cout << "Retrying...";
			// Retry connecting
			StartGuacClient();
		}
		std::cout << std::endl;
	} else if(internal_state_ == InternalState::kConnected) {
		// Check if the user initiated the disconnect
		if(guac_client_.GetDisconnectReason() != GuacClient::DisconnectReason::kClient) {
			std::cout << "Guacamole client unexpectedly disconnected (Code: " << guac_client_.GetDisconnectReason() << "). Reconnecting..." << std::endl;
		}
		internal_state_ = InternalState::kVNCConnecting;
		// Reset retry counter
		retry_count_ = 0;
		// Attempt to reconnect
		StartGuacClient();
	}
}

void QEMUController::CleanUp() {
	guac_client_.CleanUp();

	GuacDisconnect();
}

void QEMUController::OnGuacStarted() {
	if(internal_state_ == InternalState::kVNCConnecting) {
		guac_client_.Connect();
	}
}

void QEMUController::OnGuacStopped() {
	if(internal_state_ == InternalState::kStopping) {
		IsStopped();
	}
	//else if (internal_state_ == InternalState::kVNCConnecting)
	//{
	//	// This shouldn't happen
	//	guac_client_.Start();
	//}
}

void QEMUController::OnGuacConnect() {
	if(internal_state_ == InternalState::kVNCConnecting) {
		internal_state_ = InternalState::kConnected;
		//server_.OnVMControllerStart(shared_from_this());
		server_.OnVMControllerStateChange(shared_from_this(), VMController::ControllerState::kRunning);
	} else {
		guac_client_.Disconnect();
	}
}

void QEMUController::OnGuacDisconnect(bool cleanup) {
	if(cleanup)
		server_.OnVMControllerCleanUp(shared_from_this());
	else
		GuacDisconnect();
}

void QEMUController::SendMonitorCommand(std::string cmd, QMPClient::ResultCallback resultCb) {
	//if (internal_state_ == InternalState::kConnected)
	qmp_->SendMonitorCommand(cmd, resultCb);
}

std::uint8_t QEMUController::GetKind() const {
	return VMSettings::HypervisorEnum::kQEMU;
}

void QEMUController::OnQMPStateChange(QMPClient::QMPState state) {
	switch(state) {
		case QMPClient::QMPState::kConnected:
			if(internal_state_ == InternalState::kQMPConnecting) {
				std::cout << "Connected to QMP" << std::endl;
#ifndef _WIN32
				// Now that we know the QEMU process has started, let's renice it and all its threads.
				constexpr auto NICE_LEVEL = 19;
				ReniceAllTasks(qemu_pid_, NICE_LEVEL);
#endif

				// It's possible for the VNC client to already be connected
				// if the QMP client disconnected after the VNC client was connected
				if(guac_client_.GetState() != GuacClient::ClientState::kConnected) {
					internal_state_ = InternalState::kVNCConnecting;
					StartGuacClient();
				}
			} else
				qmp_->Disconnect();
			break;
		default:
			//case QMPClient::States::kDisconnected:
			if(internal_state_ == InternalState::kStopping) {
#ifdef _WIN32
				// For debugging on Windows
				qemu_running_ = false;
#endif
				IsStopped();
			} else if(internal_state_ == InternalState::kQMPConnecting) {
				std::cout << "QMP failed to connect. ";
				// If we have exceeded the max number of connection attempts
				if(++retry_count_ >= settings_->MaxAttempts) {
					std::cout << "Max number attempts has been exceeded. Stopping..." << std::endl;

					error_code_ = ErrorCode::kQMPFailed;
					Stop(StopReason::kError);
				} else {
					std::cout << "Retrying..." << std::endl;
					// Retry connecting
					//KillQEMU();
					//StartQEMU();
					StartQMP();
				}
			} else if(internal_state_ == InternalState::kVNCConnecting ||
					  internal_state_ == InternalState::kConnected) {
				std::cout << "QMP unexpectedly disconnected. Reconnecting..." << std::endl;
				internal_state_ = InternalState::kQMPConnecting;
				// Reset retry counter
				retry_count_ = 0;
				StartQMP();
			}
	}
}

std::vector<const char*> QEMUController::SplitCommandLine(const char* command) {
	std::vector<const char*> cmdList;
	int i;

	// Posix.
#ifndef _WIN32
	{
		wordexp_t p;

		// Note! This expands shell variables.
		if(wordexp(command, &p, 0)) {
			return cmdList;
		}

		cmdList.reserve(p.we_wordc);

		for(i = 0; i < p.we_wordc; i++) {
			int len = strlen(p.we_wordv[i]) + 1;
			if(!len)
				continue;
			char* arg = new char[len];
			memcpy(arg, p.we_wordv[i], len);

			cmdList.push_back(arg);
		}

	fail:
		wordfree(&p);

		return cmdList;
	}
#else // WIN32
	{
		// Windows has CommandLineToArgvW but not CommandLineToArgvA
		// so the command must be converted to unicode first
		wchar_t** wargs = NULL;
		size_t needed = 0;
		size_t len = strlen(command) + 1;
		wchar_t* cmdLineW = new wchar_t[len];

		if(!MultiByteToWideChar(CP_ACP, 0, command, -1, cmdLineW, len))
			goto fail;

		int argc;
		if(!(wargs = CommandLineToArgvW(cmdLineW, &argc)))
			goto fail;

		cmdList.reserve(argc);

		// Convert from wchar_t * to ANSI char *
		for(i = 0; i < argc; i++) {
			// Get the size needed for the target buffer.
			// CP_ACP = Ansi Codepage.
			needed = WideCharToMultiByte(CP_ACP, 0, wargs[i], -1,
										 NULL, 0, NULL, NULL);

			char* arg = new char[needed];

			// Do the conversion.
			needed = WideCharToMultiByte(CP_ACP, 0, wargs[i], -1,
										 arg, needed, NULL, NULL);

			cmdList.push_back(arg);
		}
	fail:
		if(wargs)
			LocalFree(wargs);
		if(cmdLineW)
			delete[] cmdLineW;
		return cmdList;
	}
#endif // WIN32
}


const std::string& QEMUController::GetErrorMessage() const {
	return kErrorMessages[error_code_];
}

VMController::ControllerState QEMUController::GetState() const {
	// Convert internal state into VMController state
	switch(internal_state_) {
		case InternalState::kVNCConnecting:
		case InternalState::kQMPConnecting:
			return ControllerState::kStarting;
		case InternalState::kConnected:
			return ControllerState::kRunning;
		case InternalState::kStopping:
			return ControllerState::kStopping;
		case InternalState::kInactive:
		default:
			return ControllerState::kStopped;
	}
}

void QEMUController::OnAddUser(CollabVMUser& user) {
	user.vm_controller = this;
	guac_client_.AddUser(*user.guac_user);
}

void QEMUController::OnRemoveUser(CollabVMUser& user) {
	guac_client_.RemoveUser(*user.guac_user);
	user.vm_controller = nullptr;
}
