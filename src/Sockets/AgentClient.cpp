#include "AgentClient.h"
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <functional>
#include <limits>
#include <streambuf>
#include <locale>
#include <codecvt>

AgentClient::AgentClient(boost::asio::io_service& service) :
	state_(ConnectionState::kNotConnected),
	timer_(service)
{
}

void AgentClient::UploadFile(const std::shared_ptr<UploadInfo>& info)
{
	auto self = shared_from_this();
	GetService().dispatch([this, self, info]()
	{
		if (file_upload_state_ == UploadState::kNotUploading &&
			(state_ == ConnectionState::kBody || state_ == ConnectionState::kHeader) &&
			info->file_stream)
		{
			info->file_stream.seekg(0, std::fstream::end);
			std::ifstream::streamoff upload_size = info->file_stream.tellg();
			if (upload_size > 0 && upload_size < std::numeric_limits<uint32_t>::max())
			{
				try
				{
					std::wstring_convert<std::codecvt_utf8_utf16<utf16_t>, utf16_t> cv;
					winstring filename_u16 = cv.from_bytes(info->filename);
					size_t filename_size = filename_u16.length();
					if (filename_size && filename_size <= max_path_component_)
					{
						filename_size *= sizeof(uint16_t);
						file_upload_state_ = UploadState::kCreateFile;
						file_upload_info_ = info;

						uint8_t* p = write_buf_;
						AgentProtocol::WriteUint16(sizeof(uint8_t) +
												   filename_size +
												   sizeof(uint32_t), &p);
						AgentProtocol::WriteUint8(AgentProtocol::ServerOpcode::kFileDlBegin, &p);
						AgentProtocol::WriteUint8(filename_u16.length(), &p);
						std::memcpy(p, filename_u16.c_str(), filename_size);
						p += filename_size;
						AgentProtocol::WriteUint32(upload_size, &p);
						std::shared_ptr<SocketCtx> ctx = GetSocketContext();
						DoWrite(write_buf_, p - write_buf_,
								std::bind(&AgentClient::OnWrite, shared_from_this(),
										  std::placeholders::_1, std::placeholders::_2, ctx));
						return;
					}
				}
				catch (std::range_error& err)
				{
					// Failed to convert filename from UTF-8 to UTF-16
				}
			}
		}

		if (auto ptr = controller_.lock())
			ptr->OnFileUploadFailed(file_upload_info_);
	});
}

/**
 * This function receives the max filename length from the caller so that a
 * shorter length can be provided if desired. It also prevents a data race
 * with max_path_component if this function is called from a different thread.
 */
bool AgentClient::IsFilenameValid(uint32_t max_len, const std::string& filename)
{
	// Pre-conversion check:
	// A code point in UTF-8 can be represented with up to 4 code units
	if (!filename.empty() && filename.length() <= max_len * 4)
	{
		try
		{
			std::wstring_convert<std::codecvt_utf8_utf16<utf16_t>, utf16_t> cv;
			winstring filename_u16 = cv.from_bytes(filename);
			return !filename_u16.empty() && filename_u16.length() <= max_len;
		}
		catch (std::range_error& err)
		{
			// Conversion failed
		}
	}
	return false;
}

//void AgentClient::UploadFile(const std::string& file_path, const std::string& filename, bool exec, const std::string* args, bool hide_window)
//{
//	std::string args_str;
//	if (args && !args->empty())
//		args_str = *args;
//
//	auto self = shared_from_this();
//	GetService().dispatch([this, self, file_path, filename, exec, args_str, hide_window]()
//	{
//		// Ensure that another upload is not already in progress
//		if (upload_stream_.is_open())
//			return;
//
//		upload_stream_.open(file_path, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
//		if (upload_stream_)
//		{
//			UploadFile(filename, exec, args_str, hide_window);
//		}
//	});
//}

//void AgentClient::UploadFile(const std::string& filename, bool exec, const std::string& args, bool hide_window)
//{
//	if (upload_stream_)
//	{
//		std::ifstream::streamoff upload_size = upload_stream_.tellg();
//		if (upload_size > 0 && upload_size < std::numeric_limits<uint32_t>::max())
//		{
//			try
//			{
//				std::wstring_convert<std::codecvt_utf8_utf16<utf16_t>, utf16_t> cv;
//				winstring filename_u16 = cv.from_bytes(filename);
//				size_t filename_size = filename_u16.length() * sizeof(uint16_t);
//				if (!filename_size)
//					return;
//
//				if (file_upload_info_.exec_file = exec)
//				{
//					file_upload_info_.exec_args = cv.from_bytes(args);
//					if (file_upload_info_.exec_args.length() > std::numeric_limits<uint8_t>::max())
//						return;
//					file_upload_info_.hide_window = hide_window;
//				}
//				file_upload_info_.state = FileUploadInfo::UploadState::kCreateFile;
//
//				uint8_t* p = write_buf_;
//				AgentProtocol::WriteUint16(sizeof(uint8_t) +
//										   filename_size +
//										   sizeof(uint32_t), &p);
//				AgentProtocol::WriteUint8(AgentProtocol::ServerOpcode::kFileDlBegin, &p);
//				AgentProtocol::WriteUint8(filename_u16.length(), &p);
//				std::memcpy(p, filename_u16.c_str(), filename_size);
//				p += filename_size;
//				AgentProtocol::WriteUint32(upload_size, &p);
//				std::shared_ptr<SocketCtx> ctx = GetSocketContext();
//				DoWrite(write_buf_, p - write_buf_,
//							 std::bind(&AgentClient::OnWrite, shared_from_this(),
//									   std::placeholders::_1, std::placeholders::_2, ctx));
//			}
//			catch (std::range_error& err)
//			{
//				// Failed to convert filename from UTF-8 to UTF-16
//			}
//		}
//	}
//}

AgentClient::~AgentClient()
{
}

void AgentClient::OnConnect(std::shared_ptr<SocketCtx>& ctx)
{
	state_ = ConnectionState::kAgentUpload;

	// Read agent upload request
	DoRead(1, std::bind(&AgentClient::OnReadUploadReq, shared_from_this(),
		std::placeholders::_1, std::placeholders::_2, ctx));

	//boost::system::error_code ec;
	//OnWriteAgentUpload(ec, 1, ctx);
}

void AgentClient::OnDisconnect()
{
	ConnectionState prev_state = state_;
	state_ = ConnectionState::kNotConnected;
	if (auto ptr = controller_.lock())
	{
		if (file_upload_state_ != UploadState::kNotUploading)
			ptr->OnFileUploadFailed(file_upload_info_);

		ptr->OnAgentDisconnect(prev_state != ConnectionState::kConnecting);
	}
}

void AgentClient::OnReadUploadReq(const boost::system::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx)
{
	if (ctx->IsStopped())
		return;

	if (ec || !size)
	{
		DisconnectSocket();
		return;
	}

	// Only one opcode is allowed in this state
	// and prevent multiple uploads from being started
	if (read_buf_[0] == AgentProtocol::ClientOpcode::kGetAgent && !upload_stream_.is_open())
	{
		// Open the file with the position at the end so we can get the file size
		upload_stream_.open(agent_path_, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
		if (upload_stream_.is_open() && upload_stream_.good())
		{
			std::ifstream::streamoff agent_size = upload_stream_.tellg();
			if (agent_size > 0 && agent_size < std::numeric_limits<uint32_t>::max())
			{
				uint8_t* p = write_buf_;
				AgentProtocol::WriteUint32(agent_size, &p);
				upload_stream_.seekg(0);
				upload_stream_.read(reinterpret_cast<char*>(p), AgentProtocol::kBufferSize - (p - write_buf_));
				p += upload_stream_.gcount();
				DoWrite(write_buf_, p - write_buf_, std::bind(&AgentClient::OnWriteAgentUpload,
					shared_from_this(), std::placeholders::_1, std::placeholders::_2, ctx));

				// TODO: Disconnect if client sends data during upload
			}
		}
		else
		{
			std::cout << "Error: Could not open file \"" << agent_path_ << "\" for CollabVM Agent" << std::endl;
		}
	}
}

void AgentClient::OnReadHeader(const boost::system::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx)
{
	if (ctx->IsStopped())
		return;

	if (ec || size != AgentProtocol::kHeaderSize)
	{
		DisconnectSocket();
		return;
	}

	uint8_t* p = read_buf_;
	uint16_t packet_size = AgentProtocol::ReadUint16(&p);
	if (packet_size > AgentProtocol::kBodySize)
	{
		DisconnectSocket();
		return;
	}
	packet_size_ = packet_size;
	packet_opcode_ = AgentProtocol::ReadUint8(&p);
	if (packet_size_)
	{
		DoRead(packet_size_, std::bind(&AgentClient::OnReadBody, shared_from_this(),
			std::placeholders::_1, std::placeholders::_2, ctx));
	}
	else
		OnReadBody(ec, 0, ctx);
}

void AgentClient::OnReadBody(const boost::system::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx)
{
	if (ctx->IsStopped())
		return;

	if (ec || size != packet_size_)
	{
		DisconnectSocket();
		return;
	}

	uint8_t* p = read_buf_;
	using AgentProtocol::ClientOpcode;

	if (connect_received_)
	{
		switch (packet_opcode_)
		{
		case ClientOpcode::kHeartbeat:
		{
			if (heartbeat_timeout_)
			{
				boost::system::error_code err;
				timer_.expires_from_now(std::chrono::seconds(heartbeat_timeout_), err);
				timer_.async_wait(std::bind(&AgentClient::OnHeartbeatTimeout, shared_from_this(), std::placeholders::_1));
			}
			break;
		}
		case ClientOpcode::kFileCreateFailed:
			if (file_upload_state_ == UploadState::kCreateFile)
			{
				file_upload_state_ = UploadState::kNotUploading;
				if (auto ptr = controller_.lock())
					ptr->OnFileUploadFailed(file_upload_info_);
			}
			break;
		case ClientOpcode::kFileCreationNewName:
			if (file_upload_state_ == UploadState::kCreateFile)
			{
				file_upload_state_ = UploadState::kUploadFile;
				uint8_t len = AgentProtocol::ReadUint8(&p) * 2;
				if (len || p + len <= read_buf_ + packet_size_)
				{
					try
					{
						std::wstring_convert<std::codecvt_utf8_utf16<utf16_t>, utf16_t> cv;
						std::string filename = cv.to_bytes(winstring(reinterpret_cast<utf16_t*>(p), len / 2));

						if (auto ptr = controller_.lock())
							ptr->OnFileUploadStarted(file_upload_info_, &filename);

						WriteFileBytes(ctx);
						break;
					}
					catch (std::range_error& err)
					{
					}
				}
			}
			DisconnectSocket();
			return;
		case ClientOpcode::kShellExecResult:
			if (file_upload_state_ == UploadState::kExecFile)
			{
				file_upload_state_ = UploadState::kNotUploading;

				int hInst = static_cast<int>(AgentProtocol::ReadUint32(&p));

				// There should not be any data left over
				if (p != read_buf_ + packet_size_)
				{
					DisconnectSocket();
					return;
				}

				if (auto ptr = controller_.lock())
					ptr->OnFileUploadExecFinished(file_upload_info_, hInst > 32);

				break;
			}
		case ClientOpcode::kFileCreationSuccess:
		{
			if (file_upload_state_ == UploadState::kCreateFile)
			{
				file_upload_state_ = UploadState::kUploadFile;

				if (auto ptr = controller_.lock())
					ptr->OnFileUploadStarted(file_upload_info_, nullptr);

				WriteFileBytes(ctx);
				break;
			}
			// Fall through
		}
		default:
			DisconnectSocket();
			return;
		}
	}
	else if (packet_opcode_ == ClientOpcode::kConnect)
	{
		max_path_component_ = AgentProtocol::ReadUint32(&p);

		bool valid_str;
		std::string os_name = ReadStringU16(&p, read_buf_ + packet_size_, valid_str);
		if (!valid_str)
		{
			DisconnectSocket();
			return;
		}

		std::string service_pack = ReadStringU16(&p, read_buf_ + packet_size_, valid_str);
		if (!valid_str)
		{
			DisconnectSocket();
			return;
		}

		std::string pc_name = ReadStringU16(&p, read_buf_ + packet_size_, valid_str);
		if (!valid_str)
		{
			DisconnectSocket();
			return;
		}

		std::string username = ReadStringU16(&p, read_buf_ + packet_size_, valid_str);
		if (!valid_str)
		{
			DisconnectSocket();
			return;
		}

		// There should not be any data left over
		if (p != read_buf_ + packet_size_)
		{
			DisconnectSocket();
			return;
		}

		connect_received_ = true;

		if (auto ptr = controller_.lock())
			ptr->OnAgentConnect(os_name, service_pack, pc_name, username, max_path_component_);
	}
	else
	{
		// First packet was not connect
		DisconnectSocket();
		return;
	}

	DoRead(AgentProtocol::kHeaderSize, std::bind(&AgentClient::OnReadHeader,
		shared_from_this(), std::placeholders::_1, std::placeholders::_2, ctx));
}

std::string AgentClient::ReadStringU16(uint8_t** p, uint8_t* end, bool& valid)
{
	try
	{
		uint8_t len = AgentProtocol::ReadUint8(p) * 2;
		if (*p + len <= end)
		{
			std::wstring_convert<std::codecvt_utf8_utf16<utf16_t>, utf16_t> cv;
			std::string str = cv.to_bytes(winstring(reinterpret_cast<utf16_t*>(*p), len / 2));
			*p += len;
			valid = true;
			return str;
		}
	}
	catch (std::range_error& err)
	{
	}

	valid = false;
	return std::string();
}

void AgentClient::OnWrite(const boost::system::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx)
{
	if (ctx->IsStopped())
		return;

	if (ec)
	{
		DisconnectSocket();
		return;
	}
}

void AgentClient::OnWriteAgentUpload(const boost::system::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx)
{
	if (ctx->IsStopped())
		return;

	if (ec || !size)
	{
		DisconnectSocket();
		return;
	}

	if (upload_stream_)
	{
		upload_stream_.read(reinterpret_cast<char*>(write_buf_), AgentProtocol::kBufferSize);
		std::streamsize read = upload_stream_.gcount();
		if (read)
		{
			DoWrite(write_buf_, read,
					std::bind(&AgentClient::OnWriteAgentUpload, shared_from_this(),
							  std::placeholders::_1, std::placeholders::_2, ctx));
			return;
		}
	}

	upload_stream_.close();
	state_ = ConnectionState::kHeader;

	DoRead(AgentProtocol::kHeaderSize,
		   std::bind(&AgentClient::OnReadHeader, shared_from_this(),
					 std::placeholders::_1, std::placeholders::_2, ctx));

	if (heartbeat_timeout_)
	{
		boost::system::error_code err;
		timer_.expires_from_now(std::chrono::seconds(heartbeat_timeout_), err);
		timer_.async_wait(std::bind(&AgentClient::OnHeartbeatTimeout, shared_from_this(), std::placeholders::_1));
	}
}

void AgentClient::WriteFileBytes(std::shared_ptr<SocketCtx>& ctx)
{
	// Write opcode
	uint8_t* write = write_buf_ + sizeof(uint16_t);
	AgentProtocol::WriteUint8(AgentProtocol::ServerOpcode::kFileDlPart, &write);
	// Write file bytes
	std::fstream& stream = file_upload_info_->file_stream;
	stream.seekg(0);
	stream.read(reinterpret_cast<char*>(write), AgentProtocol::kBufferSize - (write - write_buf_));
	uint16_t read = stream.gcount();
	// Write packet size
	AgentProtocol::WriteUint16(read, write_buf_);

	DoWrite(write_buf_, AgentProtocol::kHeaderSize + read,
			std::bind(&AgentClient::OnWriteFileUpload, shared_from_this(),
					  std::placeholders::_1, std::placeholders::_2, ctx));
}

void AgentClient::OnWriteFileUpload(const boost::system::error_code& ec, size_t size, std::shared_ptr<SocketCtx> ctx)
{
	if (ctx->IsStopped())
		return;

	if (ec)
	{
		DisconnectSocket();
		return;
	}

	std::fstream& stream = file_upload_info_->file_stream;
	if (stream)
	{
		// Write opcode
		uint8_t* p = write_buf_ + sizeof(uint16_t);
		AgentProtocol::WriteUint8(AgentProtocol::ServerOpcode::kFileDlPart, &p);
		// Write file bytes
		stream.read(reinterpret_cast<char*>(p), AgentProtocol::kBufferSize - (p - write_buf_));
		uint16_t read = stream.gcount();

		if (read)
		{
			// Write packet size
			AgentProtocol::WriteUint16(read, write_buf_);

			DoWrite(write_buf_, AgentProtocol::kHeaderSize + read,
					std::bind(&AgentClient::OnWriteFileUpload, shared_from_this(),
							  std::placeholders::_1, std::placeholders::_2, ctx));
			return;
		}
	}

	stream.close();

	if (file_upload_info_->run_file)
	{
		file_upload_state_ = UploadState::kExecFile;

		uint8_t* p = write_buf_ + sizeof(uint16_t);
		AgentProtocol::WriteUint8(AgentProtocol::ServerOpcode::kFileDlEndShellExec, &p);
		AgentProtocol::WriteUint8(0, &p);
		//AgentProtocol::WriteUint8(file_upload_info_.exec_args.length(), &p);
		//if (!file_upload_info_.exec_args.empty())
		//{
		//	std::memcpy(p, file_upload_info_.exec_args.c_str(),
		//				file_upload_info_.exec_args.length() * sizeof(uint16_t));
		//	p += file_upload_info_.exec_args.length() * sizeof(uint16_t);
		//}
		//AgentProtocol::WriteUint8(file_upload_info_.hide_window ?
		//						  AgentProtocol::ShowWindow::kHide : AgentProtocol::ShowWindow::kShow, &p);
		AgentProtocol::WriteUint8(AgentProtocol::ShowWindow::kShow, &p);
		AgentProtocol::WriteUint16(p - write_buf_ - AgentProtocol::kHeaderSize, write_buf_);
		DoWrite(write_buf_, p - write_buf_,
				std::bind(&AgentClient::OnWrite, shared_from_this(),
						  std::placeholders::_1, std::placeholders::_2, ctx));
	}
	else
	{
		file_upload_state_ = UploadState::kNotUploading;

		uint8_t* p = write_buf_;
		AgentProtocol::WriteUint16(0, &p);
		AgentProtocol::WriteUint8(AgentProtocol::ServerOpcode::kFileDlEnd, &p);
		DoWrite(write_buf_, p - write_buf_,
				std::bind(&AgentClient::OnWrite, shared_from_this(),
						  std::placeholders::_1, std::placeholders::_2, ctx));

		if (auto ptr = controller_.lock())
			ptr->OnFileUploadFinished(file_upload_info_);
	}
}

void AgentClient::OnHeartbeatTimeout(const boost::system::error_code& ec)
{
	if (ec)
		return;

	if (auto ptr = controller_.lock())
		ptr->OnAgentHeartbeatTimeout();
}
