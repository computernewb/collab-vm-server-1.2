#pragma once
#include <stdint.h>
#include <string>
#include <functional>
#include <map>
#include <chrono>

#include "Sockets/TCPSocketClient.h"
#include "Sockets/LocalSocketClient.h"
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>
#include "rapidjson/document.h"

class QMPCallback;
/**
 * Controls QEMU using the QEMU Machine Protocol (QMP). QMP uses JSON
 * and it allows the controller to be notified of events when they occur.
 */
class QMPClient : public std::enable_shared_from_this<QMPClient> {
   public:
	QMPClient(boost::asio::io_service& service)
		: timer_(service),
		  state_(ConnectionState::kDisconnected) {
	}

	enum class Events {
		BLOCK_IO_ERROR,
		RESET,	// Emitted when the VM is reset
		RESUME, // Emitted when the VM resumes execution
		RTC_CHANGE,
		SHUTDOWN, // Emitted when the VM is powered down
		STOP,	  // Emitted when the VM is stopped
		VNC_CONNECTED,
		VNC_DISCONNECTED,
		VNC_INITIALIZED,
		WATCHDOG
	};

	enum class QMPState {
		kConnected,		 // Connected to QEMU
		kConnectFailure, // Failed to connect to QEMU
		kProtocolError,	 // A protocol error occurred
		kDisconnected	 // Disconnected from QEMU
	};

	typedef std::function<void(rapidjson::Document&)> EventCallback;
	typedef std::function<void(bool connected)> ConnectionStateCallback;
	typedef std::function<void(rapidjson::Document&)> ResultCallback;

	/**
	 * Attempts to connect to QEMU.
	 */
	void Connect(std::weak_ptr<QMPCallback> controller);

	/**
	 * Disconnects from QEMU.
	 */
	void Disconnect();

	/**
	 * Registers a function to be called when the specified event
	 * occurs.
	 */
	void RegisterEventCallback(Events event, EventCallback callback);

	/**
	 * Sends the system_powerdown command to QEMU.
	 * This should cause the RESET event to occur.
	 */
	void SystemPowerDown();

	/**
	 * Sends the system_reset command to QEMU.
	 * This should cause the RESET event to occur.
	 */
	void SystemReset();

	/**
	 * Sends the stop command to QEMU.
	 * This should cause the STOP event to occur.
	 */
	void SystemStop();

	/**
	 * Sends the cont command to QEMU.
	 * This should cause the RESUME event to occur.
	 */
	void SystemResume();

	/**
	 * Sends the quit command to QEMU.
	 * This should cause the SHUTDOWN and STOP events to occur.
	 */
	void QEMUQuit();

	void LoadSnapshot(const std::string& snapshot, ResultCallback result_cb);
	void SendMonitorCommand(const std::string& cmd, ResultCallback result_cb);

	/**
	 * Whether the client is connected and the handshake is complete.
	 */
	bool IsConnected() const {
		return state_ == ConnectionState::kConnected;
	}

   protected:
	void OnConnect(std::shared_ptr<SocketCtx>& ctx);
	void OnDisconnect();

	virtual void ConnectSocket() = 0;
	virtual void DisconnectSocket() = 0;

	virtual void DoReadLine(std::shared_ptr<SocketCtx>& ctx) = 0;
	virtual void DoWriteData(const char data[], size_t len, std::shared_ptr<SocketCtx>& ctx) = 0;

	template<typename SocketType>
	void ReadLine(SocketType& socket, std::shared_ptr<SocketCtx>& ctx) {
		boost::asio::async_read_until(socket, buf_, "\r\n",
									  std::bind(&QMPClient::OnReadLine, shared_from_this(), std::placeholders::_1, std::placeholders::_2, ctx));
	}

	template<typename SocketType>
	void WriteData(SocketType& socket, const char data[], size_t len, std::shared_ptr<SocketCtx>& ctx) {
		boost::asio::async_write(socket, boost::asio::buffer(data, len),
								 std::bind(&QMPClient::OnWrite, shared_from_this(), std::placeholders::_1, std::placeholders::_2, ctx));
	}

	virtual boost::asio::io_service& GetService() = 0;
	virtual std::shared_ptr<SocketCtx>& GetSocketContext() = 0;

   private:
#ifdef USE_SYSTEM_CLOCK
	typedef std::chrono::system_clock time_clock;
#else
	typedef std::chrono::steady_clock time_clock;
#endif

	void SendString(const std::string& str);
	void OnReadLine(const boost::system::error_code& ec, size_t size, std::shared_ptr<SocketCtx>& ctx);
	void OnWrite(const boost::system::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx);
	void StartTimeoutTimer();

	enum class ConnectionState {
		kDisconnected,
		kConnecting,   // Handshake not started
		kCapabilities, // Waiting for "QMP" command for capability negotiation
		kResponse,	   // Waiting for "return" command to confirm negotiation
		kConnected	   // Done negotiating
	};

	boost::asio::steady_timer timer_;
	std::weak_ptr<QMPCallback> controller_;
	boost::asio::streambuf buf_;

	ConnectionState state_;

	const std::string events_[10] = {
		"BLOCK_IO_ERROR",
		"RESET",
		"RESUME",
		"RTC_CHANGE",
		"SHUTDOWN",
		"STOP",
		"VNC_CONNECTED",
		"VNC_DISCONNECTED",
		"VNC_INITIALIZED",
		"WATCHDOG"
	};

	EventCallback event_callbacks_[10];

	std::map<uint16_t, ResultCallback> result_callbacks_;
	uint16_t result_id_;

	const std::chrono::seconds kReadTimeout = std::chrono::seconds(3);
};

class QMPCallback {
   public:
	virtual void OnQMPStateChange(QMPClient::QMPState state) = 0;
};

template<typename Socket>
class QMPClientSocket : public Socket {
   public:
	QMPClientSocket(boost::asio::io_service& service, const std::string& host, uint16_t port)
		: TCPSocketClient<QMPClient>(service, host, port) {
	}

	QMPClientSocket(boost::asio::io_service& service, const std::string& name)
		: LocalSocketClient<QMPClient>(service, name) {
	}

   protected:
	void DoReadLine(std::shared_ptr<SocketCtx>& ctx) override {
		Socket::ReadLine(Socket::GetSocket(), ctx);
	}

	void DoWriteData(const char data[], size_t len, std::shared_ptr<SocketCtx>& ctx) override {
		Socket::WriteData(Socket::GetSocket(), data, len, ctx);
	}
};

typedef QMPClientSocket<TCPSocketClient<QMPClient>> QMPTCPClient;
typedef QMPClientSocket<LocalSocketClient<QMPClient>> QMPLocalClient;
