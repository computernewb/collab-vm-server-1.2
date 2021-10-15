#include "VMControllers/QEMUController.h"
#include "CollabVM.h"

#include <util/CommandLine.h>

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


QEMUController::QEMUController(CollabVMServer& server, boost::asio::io_service& service, const std::shared_ptr<VMSettings>& settings)
	: VMController(server, service, settings),
	  guac_client_(server, *this, users_, settings->VNCAddress, settings->VNCPort),
	  internal_state_(InternalState::kInactive),
	  qemu_running_(false),
	  timer_(service),
	  retry_count_(0),
	  ioc(service)
{
	SetCommand(settings->QEMUCmd);

	std::lock_guard<std::mutex> lock(qmp_thread_mutex_);
	if(!qmp_count_++) {
		// TODO: asio::io_service::work is deprecated, and we really 
		// should do something better.
		// Also, instead of new/delete, we should use some deferring initalizer type

		qmp_service_ = new boost::asio::io_service();

		qmp_work_ = new boost::asio::io_service::work(*qmp_service_);
		qmp_thread_ = std::thread([](boost::asio::io_service* service) {
			service->run();
			delete service;
		}, qmp_service_);

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

void QEMUController::OnChildExit(int exitCode, const std::error_code& ec) {
	if(ec)
		return;

	// maybe pid might work on exit?
	// unless it fetches it from waitpid

	std::cout << "QEMU child process exited with status code " << exitCode << '\n';

	qemu_running_ = false;

	// Stop the timer
	boost::system::error_code boost_ec;
	timer_.cancel(boost_ec);

	if(internal_state_ == InternalState::kStopping) {
		IsStopped();
		return;
	}

	if(internal_state_ != InternalState::kInactive) {
		// If QEMU's exit code is not zero and QMP is not currently connected,
		// it probably means that it was never able to connect,
		// because QEMU was started with invalid arguments.

#if 0 // TODO: We should get this code to work again.
		if(exitCode != 0) {
			internal_state_ = InternalState::kStopping;
			error_code_ = ErrorCode::kQEMUError;
			Stop(StopReason::kError);
			IsStopped();

			std::cout << "QEMU terminated with a non-zero status code which indicates an error. Check the command for any invalid arguments.\n";
		} else {
			// Restart QEMU
			StartQEMU();
		}
#else
		// just restart for right now
		// maybe if there are too many bad restarts we stop the VM,
		// but idk

		StartQEMU();
#endif
	}
}


void QEMUController::Start() {
	if(internal_state_ != InternalState::kInactive)
		return;

	server_.OnVMControllerStateChange(shared_from_this(), VMController::ControllerState::kStarting);

	std::weak_ptr<QEMUController> con(std::static_pointer_cast<QEMUController>(shared_from_this()));

	// The STOP event will be received after the SHUTDOWN
	// event when the -no-shutdown argument is provided
	qmp_->RegisterEventCallback(QMPClient::Events::STOP, [con](rapidjson::Document& d) {
		auto ptr = con.lock();
		if(!ptr)
			return;

		std::cout << "[QEMU] Stop event occurred" << std::endl;

		if(ptr->internal_state_ != InternalState::kStopping) {
			if(ptr->settings_->RestoreOnShutdown && ptr->settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kHDSnapshots /* || (ptr->settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kVMSnapshots && ptr->RestartForSnapshot)*/ ) {
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
				ptr->timer_.async_wait(std::bind(&QEMUController::ProcessKillTimeout,  std::static_pointer_cast<QEMUController>(ptr), std::placeholders::_1));

				ptr->qmp_->SystemReset();
			}
		}
	});

	qmp_->RegisterEventCallback(QMPClient::Events::RESET, [con](rapidjson::Document& d) {
		std::cout << "[QEMU] Reset event occurred" << std::endl;

		auto ptr = con.lock();
		if(!ptr)
			return;

		// Cancel the timeout timer
		boost::system::error_code ec;
		ptr->timer_.cancel(ec);

		// After QEMU has been reset, load the snapshot
		if(ptr->settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kVMSnapshots && !ptr->snapshot_.empty()) {
			if(ptr->RestartForSnapshot) {
				// This shouldn't happen
				// The QEMU process must be restarted
				// Restart QEMU to restore the snapshot
				std::cout << "[QEMU] Restarting QEMU..." << std::endl;

				ptr->StopQEMU();
			} else {
				// Send the loadvm command to the monitor to restore the snapshot
				ptr->qmp_->SendMonitorCommand("loadvm " + ptr->snapshot_, [con](rapidjson::Document&) {
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


	StartQEMU();
}

void QEMUController::SetCommand(const std::string& command) {
	auto cmdopt = collabvm::util::SplitCommandLine(command);

	if(!cmdopt.has_value()) {
		std::cout << "I'm not sure what you've done, but you've made collabvm::util::SplitCommandLine return a nullopt. Good job, you get a cookie.\n";
		return;
	}

	qemu_command_ = cmdopt.value();
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

			// TODO: it might be nice to use /run/collabvm
			// or /tmp/collabvm/. Cluttering /tmp with qemu sockets is kind of lame

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

	// Make an intentional copy of the QEMU command line,
	// so we can add args to it.
	auto args_copy = qemu_command_;

	if(settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kVMSnapshots && !snapshot_.empty()) {
		// Append loadvm command to start with snapshot
		args_copy.push_back("-loadvm");
		args_copy.push_back(snapshot_);
	} else if(settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::kHDSnapshots) {
		args_copy.push_back("-snapshot");
	}

	//if (settings_->QEMUSnapshotMode == VMSettings::SnapshotMode::)
	args_copy.push_back("-no-shutdown");

	// QMP address
	args_copy.push_back("-qmp");

	std::string qmp_arg;
	if(settings_->QMPSocketType == VMSettings::SocketType::kTCP) {
		qmp_arg = "tcp:";
		qmp_arg += qmp_address_;
		qmp_arg += ",server,nodelay";

		args_copy.push_back(qmp_arg);
	} else {
		qmp_arg = "unix:";
		qmp_arg += qmp_address_;
		qmp_arg += ",server";

		args_copy.push_back(qmp_arg);
	}

// this'll still be okay to keep around
#if 0
	std::string arg;
	if(settings_->AgentEnabled) {
		if(settings_->AgentUseVirtio) {
			// -chardev socket,id=agent,host=10.0.2.15,port=5700,nodelay,server,nowait -device virtio-serial -device virtserialport,chardev=agent
			args_copy.push_back("-chardev");
			arg = "socket,id=agent,";
			if(settings_->AgentSocketType == VMSettings::SocketType::kTCP) {
				arg += "host=";
				arg += settings_->AgentAddress;
				arg += ",port=";
				arg += std::to_string(settings_->AgentPort);
				arg += ",nodelay";
			} else {
				arg += "path=";
				arg += agent_address_;
			}
			arg += ",server,nowait";
			args_copy.push_back(arg);

			args_copy.push_back("-device");
			args_copy.push_back("virtio-serial");

			args_copy.push_back("-device");
			args_copy.push_back("virtserialport,chardev=agent");
		} else {
			// Serial address
			args_copy.push_back("-serial");
			if(settings_->AgentSocketType == VMSettings::SocketType::kTCP) {
				arg = "tcp:";
				arg += agent_address_;
				// nowait is used because the AgentClient does not connect
				// until after the VM has been started with QMP
				arg += ",server,nowait,nodelay";
			} else {
				arg = "unix:";
				arg += agent_address_;
				arg += ",server,nowait";
			}
			args_copy.push_back(arg.c_str());
		}
	}
#endif

	// Append VNC argument

	args_copy.push_back("-vnc");
	std::string vnc_arg = settings_->VNCAddress + ':' + std::to_string(settings_->VNCPort - 5900);
	args_copy.push_back(vnc_arg);

	std::cout << "Starting QEMU with command:\n";

	for(auto& it : args_copy) {
		std::cout << it << ' ';
	}
	std::cout << std::endl;

	using namespace std::placeholders;

	// Spawn the QEMU process
	auto exit_handler = std::bind(&QEMUController::OnChildExit, std::static_pointer_cast<QEMUController>(shared_from_this()), _1, _2);
	qemu_child_ = collabvm::util::StartChildProcess(ioc, args_copy, std::move(exit_handler));

	qemu_running_ = true;
	internal_state_ = InternalState::kQMPConnecting;
	retry_count_ = 0;
	StartQMP();
}

void QEMUController::StopQEMU() {
	// Attempt to stop QEMU with QMP if it's connected
	if(qmp_->IsConnected()) {
		// when was this changed??
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
	//if(qemu_child_.running())
	qemu_child_.terminate();

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
	timer_.async_wait(std::bind(&QEMUController::StartQMPCallback, std::static_pointer_cast<QEMUController>(shared_from_this()), std::placeholders::_1));
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
	timer_.async_wait(std::bind(&QEMUController::StartGuacClientCallback, std::static_pointer_cast<QEMUController>(shared_from_this()), std::placeholders::_1));
}

void QEMUController::ProcessKillTimeout(const boost::system::error_code& ec) {
	if(ec)
		return;

	std::cout << "QEMU did not terminate within 5 seconds. Killing process..." << std::endl;
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

void QEMUController::OnQMPStateChange(QMPClient::QMPState state) {
	switch(state) {
		case QMPClient::QMPState::kConnected:
			if(internal_state_ == InternalState::kQMPConnecting) {
				std::cout << "Connected to QMP" << std::endl;

				// It's possible for the VNC client to already be connected
				// if the QMP client disconnected after the VNC client was connected
				if(guac_client_.GetState() != GuacClient::ClientState::kConnected) {
					internal_state_ = InternalState::kVNCConnecting;
					StartGuacClient();
				}
				
				// 
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