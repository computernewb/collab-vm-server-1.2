#pragma once
#include <string>
#include <memory>
#include <stdint.h>
#include <mutex>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <functional>
#include <map>
#include "rapidjson/document.h"

class QMPCallback;
/**
 * Controls QEMU using the QEMU Machine Protocol (QMP). QMP uses JSON
 * and it allows the controller to be notified of events when they occur.
 */
class QMP : public std::enable_shared_from_this<QMP>
{
public:
	enum Events
	{
		BLOCK_IO_ERROR,
		RESET,				// Emitted when the Virtual Machine is reset.
		RESUME,				// Emitted when the Virtual Machine resumes execution.
		RTC_CHANGE,
		SHUTDOWN,			// Emitted when the Virtual Machine is powered down.
		STOP,				// Emitted when the Virtual Machine is stopped.
		VNC_CONNECTED,
		VNC_DISCONNECTED,
		VNC_INITIALIZED,
		WATCHDOG
	};

	enum States
	{
		kConnected,			// Connected to QEMU
		kConnectFailure,	// Failed to connect to QEMU
		kProtocolError,		// A protocol error occurred
		kDisconnected		// Disconnected from QEMU
	};

	typedef std::function<void(rapidjson::Document&)> EventCallback;
	typedef std::function<void(bool connected)> ConnectionStateCallback;
	typedef std::function<void(rapidjson::Document&)> ResultCallback;

	QMP(boost::asio::io_service& service, const std::string& host, uint16_t port);

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

	void LoadSnapshot(std::string snapshot, ResultCallback resultCb);
	void SendMonitorCommand(std::string cmd, ResultCallback resultCb);
	/**
	 * Sets the host and port to connect to.
	 */
	void SetEndpoint(std::string& host, uint16_t port);

	/**
	 * Whether the client is connected and the handshake is complete.
	 */
	bool IsConnected() const
	{
		return state_ == ConnectionState::kFinished;
	}

	/**
	 * The host of the QEMU server to connect to.
	 */
	const std::string& Host() const
	{
		return host_;
	}

	/**
	 * The port of the QEMU server to connect to.
	 */
	const std::string& Port() const
	{
		return port_;
	}

private:
#ifdef USE_SYSTEM_CLOCK
	typedef std::chrono::system_clock time_clock;
#else
	typedef std::chrono::steady_clock time_clock;
#endif

	void DisconnectReason(States state);
	void TimerCallback(const boost::system::error_code& ec);
	void OnResolve(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator iterator);
	void StartConnection(boost::asio::ip::tcp::resolver::iterator endpoint_iter);
	void OnConnect(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator iterator);
	void OnReadLine(const boost::system::error_code& e, std::size_t size);
	void OnWrite(const boost::system::error_code& ec, size_t size);
	void ReadLine();
	void WriteData(const char data[], size_t len);

	enum ConnectionState
	{
		kConnecting,	// Handshake not started
		kCapabilities,	// Waiting for "QMP" command for capability negotiation
		kResponse,		// Waiting for "result" command to confirm negotiation
		kFinished		// Done negotiating
	};

	/**
	 * The QEMUController that uses this QMP instance. Callbacks will be called
	 * with this object and it shouldn't be deleted before this instance.
	 */
	std::weak_ptr<QMPCallback> controller_;
	boost::asio::ip::tcp::resolver _resolver;
	boost::asio::ip::tcp::socket socket_;
	std::string host_;
	std::string port_;
	boost::asio::streambuf buf_;
	/**
	 * This mutex is used to prevent the connect and disconnect
	 * callbacks from being called multiple times. It should be
	 * locked before changing the stopped_ member.
	 */
	std::mutex lock_;

	/**
	 * Whether a connection has been established with the server.
	 */
	bool stopped_;
	ConnectionState state_;
	boost::asio::steady_timer timer_;
	//ConnectionStateCallback state_callback_;

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

	EventCallback _eventCallbacks[10];

	std::map<uint16_t, ResultCallback> _resultCallbacks;
	uint16_t _resultId;
};


class QMPCallback
{
public:
	virtual void OnQMPStateChange(QMP::States state) = 0;
};
