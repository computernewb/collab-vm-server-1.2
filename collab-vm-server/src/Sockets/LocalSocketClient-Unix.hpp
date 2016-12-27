#pragma once
#include "SocketClient.h"

template<typename Base>
class LocalSocketClient : public SocketClient<Base, asio::local::stream_protocol::socket>
{
	typedef SocketClient<Base, asio::local::stream_protocol::socket> SC;
protected:
	LocalSocketClient(asio::io_service& service, const std::string& name) :
		SC::SocketClient(service),
		name_(name)
	{
	}

	void ConnectSocket(std::shared_ptr<SocketCtx>& ctx) override
	{
		asio::local::stream_protocol::endpoint ep(name_);
		SC::GetSocket().async_connect(ep, std::bind(&LocalSocketClient::ConnectCallback,
			std::static_pointer_cast<LocalSocketClient>(SC::shared_from_this()), std::placeholders::_1, ctx));
	}

	void Disconnect() override
	{
		asio::error_code ec;
		SC::GetSocket().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		SC::GetSocket().close(ec);
	}

private:
	void ConnectCallback(const asio::error_code& ec, std::shared_ptr<SocketCtx> ctx)
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
