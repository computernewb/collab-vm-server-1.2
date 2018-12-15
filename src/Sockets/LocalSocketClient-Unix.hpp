#pragma once
#include "SocketClient.h"

template<typename Base>
class LocalSocketClient : public SocketClient<Base, boost::asio::local::stream_protocol::socket>
{
	typedef SocketClient<Base, boost::asio::local::stream_protocol::socket> SC;
protected:
	LocalSocketClient(boost::asio::io_service& service, const std::string& name) :
		SC::SocketClient(service),
		name_(name)
	{
	}

	void ConnectSocket(std::shared_ptr<SocketCtx>& ctx) override
	{
		boost::asio::local::stream_protocol::endpoint ep(name_);
		SC::GetSocket().async_connect(ep, std::bind(&LocalSocketClient::ConnectCallback,
			std::static_pointer_cast<LocalSocketClient>(SC::shared_from_this()), std::placeholders::_1, ctx));
	}

	void Disconnect() override
	{
		boost::system::error_code ec;
		SC::GetSocket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		SC::GetSocket().close(ec);
	}

private:
	void ConnectCallback(const boost::system::error_code& ec, std::shared_ptr<SocketCtx> ctx)
	{
		if (ctx->IsStopped())
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
