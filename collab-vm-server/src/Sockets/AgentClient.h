#pragma once
#include <memory>
#include <fstream>
#include <string>
#include <stdint.h>

#include "Sockets/TCPSocketClient.h"
#include "Sockets/LocalSocketClient.h"
#include "CollabVMUser.h"
#include "UploadInfo.h"
#include <asio.hpp>

#include "AgentProtocol.h"

class AgentCallback;
typedef std::function<void(const asio::error_code& ec, size_t size)> Callback;

#ifdef _WIN32
// With MSVC the wchar_t type must be used with codecvt
typedef wchar_t utf16_t;
#else
// With GCC the char16_t type must be used with codecvt
typedef char16_t utf16_t;
#endif
static_assert(sizeof(utf16_t) == sizeof(uint16_t), "utf16_t must be exactly 16 bits");
typedef std::basic_string<utf16_t> winstring;

class AgentClient : public std::enable_shared_from_this<AgentClient>
{
public:
	AgentClient(asio::io_service& service);

	void Init(const std::string& agent_path, uint16_t heartbeat_timeout)
	{
		auto self = shared_from_this();
		GetService().dispatch([this, self, agent_path, heartbeat_timeout]()
		{
			if (state_ == ConnectionState::kNotConnected)
			{
				agent_path_ = agent_path;
				heartbeat_timeout_ = heartbeat_timeout;
			}
		});
	}

	void Connect(std::weak_ptr<AgentCallback> callback)
	{
		auto self = shared_from_this();
		GetService().dispatch([this, self, callback]()
		{
			if (state_ == ConnectionState::kNotConnected)
			{
				state_ = ConnectionState::kConnecting;
				connect_received_ = false;
				file_upload_state_ = UploadState::kNotUploading;
				controller_ = callback;
				ConnectSocket();
			}
		});
	}

	void Disconnect()
	{
		auto self = shared_from_this();
		GetService().dispatch([this, self]()
		{
			if (state_ != ConnectionState::kNotConnected)
				DisconnectSocket();
		});
	}

	/**
	 * Ownership of the UploadInfo structure is tranferred
	 * to the AgentClient and then returned to the AgentCallback.
	 * TODO: Change pointer parameter to unique_ptr to indicate ownership transfer.
	 */
	void UploadFile(const std::shared_ptr<UploadInfo>& info);

	/**
	 * Validates that the filename is not too long when converted to
	 * the target OS's filename encoding.
	 * @param max_len The maximum length of the string in UTF-16 code units.
	 * @param filename A UTF-8 encoded string.
	 */
	static bool IsFilenameValid(uint32_t max_len, const std::string& filename);

	~AgentClient();

protected:
	void OnConnect(std::shared_ptr<SocketCtx>& ctx);
	void OnDisconnect();

	virtual void ConnectSocket() = 0;
	virtual void DisconnectSocket() = 0;

	virtual void DoRead(size_t size, Callback cb) = 0;
	virtual void DoWrite(const uint8_t data[], size_t len, Callback cb) = 0;

	template<typename SocketType>
	void Read(SocketType& socket, size_t size, Callback cb)
	{
		asio::async_read(socket, asio::buffer(read_buf_, size), cb);
	}

	template<typename SocketType>
	void Write(SocketType& socket, const uint8_t data[], size_t len, Callback cb)
	{
		asio::async_write(socket, asio::buffer(data, len), cb);
	}

	virtual asio::io_service& GetService() = 0;
	virtual std::shared_ptr<SocketCtx>& GetSocketContext() = 0;

private:
	void OnReadUploadReq(const asio::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx);
	void OnReadHeader(const asio::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx);
	void OnReadBody(const asio::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx);

	void OnWrite(const asio::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx);
	void OnWriteAgentUpload(const asio::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx);
	void OnWriteFileUpload(const asio::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx);

	void OnHeartbeatTimeout(const asio::error_code& ec);

	void WriteFileBytes(std::shared_ptr<SocketCtx>& ctx);
	std::string ReadStringU16(uint8_t** p, uint8_t* end, bool& valid);

	//void UploadFile(const std::string& filename, bool exec, const std::string& args, bool hide_window);

	std::weak_ptr<AgentCallback> controller_;

	asio::steady_timer timer_;
	uint16_t heartbeat_timeout_;

	uint8_t read_buf_[AgentProtocol::kBufferSize];
	uint8_t write_buf_[AgentProtocol::kBufferSize];

	uint16_t packet_size_;
	uint8_t packet_opcode_;

	std::string agent_path_;
	std::ifstream upload_stream_;

	/**
	 * True if the client has received a connect packet from the agent.
	 */
	bool connect_received_;
	uint32_t max_path_component_;

	//struct FileUploadInfo
	//{
	//	enum class UploadState
	//	{
	//		kNotUploading,
	//		kCreateFile,
	//		kUploadFile,
	//		kExecFile
	//	} state;

	//	bool exec_file;
	//	winstring exec_args;
	//	bool hide_window;
	//	UploadInfo* info;

	//} file_upload_info_;

	enum class UploadState
	{
		kNotUploading,
		kCreateFile,
		kUploadFile,
		kExecFile
	} file_upload_state_;
	std::shared_ptr<UploadInfo> file_upload_info_;

	enum class ConnectionState
	{
		kNotConnected,
		kConnecting,
		kAgentUpload,
		kHeader,
		kBody
	} state_;
};

class AgentCallback
{
public:
	//enum DisconnectReason
	//{
	//	kConnectFailed,
	//	kHeartbeatTimeout,
	//	kExpected
	//};

	virtual void OnAgentConnect(const std::string& os_name, const std::string& service_pack,
								const std::string& pc_name, const std::string& username, uint32_t max_filename) = 0;
	virtual void OnAgentDisconnect(bool protocol_error) = 0;
	virtual void OnAgentHeartbeatTimeout() = 0;
	virtual void OnFileUploadStarted(const std::shared_ptr<UploadInfo>& info, std::string* filename) = 0;
	virtual void OnFileUploadFailed(const std::shared_ptr<UploadInfo>& info/*, Reason*/) = 0;
	virtual void OnFileUploadFinished(const std::shared_ptr<UploadInfo>& info) = 0;
	virtual void OnFileUploadExecFinished(const std::shared_ptr<UploadInfo>& info, bool exec_success) = 0;
};

template<typename Socket>
class AgentClientSocket : public Socket
{
public:
	AgentClientSocket(asio::io_service& service, const std::string& host, uint16_t port)
		: TCPSocketClient<AgentClient>(service, host, port)
	{
	}

	AgentClientSocket(asio::io_service& service, const std::string& name) :
		LocalSocketClient<AgentClient>(service, name)
	{
	}

protected:
	//virtual void DoReadUploadReq(std::shared_ptr<SocketCtx>& ctx)
	//{
	//	Socket::ReadReadUploadReq(Socket::GetSocket(), ctx);
	//}

	//virtual void DoReadHeader(std::shared_ptr<SocketCtx>& ctx)
	//{
	//	Socket::ReadHeader(Socket::GetSocket(), ctx);
	//}

	//virtual void DoReadBody(size_t size, std::shared_ptr<SocketCtx>& ctx)
	//{
	//	Socket::ReadBody(Socket::GetSocket(), size, ctx);
	//}

	void DoRead(size_t size, Callback cb) override
	{
		Socket::Read(Socket::GetSocket(), size, cb);
	}

	void DoWrite(const uint8_t data[], size_t len, Callback cb) override
	{
		Socket::Write(Socket::GetSocket(), data, len, cb);
	}
};

typedef AgentClientSocket<TCPSocketClient<AgentClient>> AgentTCPClient;
typedef AgentClientSocket<LocalSocketClient<AgentClient>> AgentLocalClient;
