#pragma once
#include "SocketClient.h"
#include <windows.h>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

template<typename Base>
class LocalSocketClient : public SocketClient<Base, boost::asio::windows::stream_handle>
{
	typedef SocketClient<Base, boost::asio::windows::stream_handle> SC;
protected:
	/**
	 * @param name The name of the pipe. Should be prefixed with "\\\\.\\pipe\\".
	 */
	LocalSocketClient(boost::asio::io_service& service, const std::string& name) :
		SC::SocketClient(service),
		name_(name)
	{
	}

	void ConnectSocket(std::shared_ptr<SocketCtx>& ctx) override
	{
		HANDLE pipe = ::CreateFileA(
			name_.c_str(),			// pipe name 
			GENERIC_READ |			// read and write access 
			GENERIC_WRITE,
			0,						// no sharing 
			NULL,					// default security attributes
			OPEN_EXISTING,			// opens existing pipe 
			FILE_FLAG_OVERLAPPED,   // overlapped IO attributes 
			NULL);					// no template file 

		if (pipe == INVALID_HANDLE_VALUE)
		{
			SC::DisconnectSocket();
			return;
		}

		boost::system::error_code ec;
		SC::GetSocket().assign(pipe, ec);

		Base::OnConnect(ctx);
	}

	void Disconnect() override
	{
		boost::system::error_code ec;
		SC::GetSocket().close(ec);
	}

private:
	void ConnectCallback(const boost::system::error_code& ec, std::shared_ptr<SocketCtx> ctx)
	{
		if (ctx->IsStopped)
			return;

		if (ec)
		{
			SC::DisconnectSocket();
			return;
		}

		Base::OnConnect(ctx);
	}

	std::string name_;
};
