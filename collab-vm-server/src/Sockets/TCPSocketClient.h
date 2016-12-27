#pragma once
#include "Sockets/SocketClient.h"
#include <functional>
#include <iostream>

template<typename Base>
class TCPSocketClient : public SocketClient<Base, asio::ip::tcp::socket>
{
	typedef SocketClient<Base, asio::ip::tcp::socket> SC;
protected:
	TCPSocketClient(asio::io_service& service, const std::string& host, uint16_t port) :
		TCPSocketClient(service, host, port, 0)
	{
	}

	TCPSocketClient(asio::io_service& service, const std::string& host, uint16_t port, uint16_t timeout) :
		SC(service),
		resolver_(service),
		timeout_timer_(service),
		host_(host),
		port_(std::to_string(port)),
		timeout_(timeout)
	{
		SC::GetSocket().open(asio::ip::tcp::v4());
		SC::GetSocket().set_option(asio::ip::tcp::no_delay(true));
	}

	void SetEndpoint(const std::string& host, uint16_t port)
	{
		host_ = host;
		port_ = std::to_string(port);
	}

	void ConnectSocket(std::shared_ptr<SocketCtx>& ctx) override
	{
		asio::ip::tcp::resolver::query query(asio::ip::tcp::v4(), host_, port_);
		resolver_.async_resolve(query, std::bind(&TCPSocketClient::OnResolve,
			std::static_pointer_cast<TCPSocketClient>(SC::shared_from_this()),
			std::placeholders::_1, std::placeholders::_2, ctx));
	}

private:
	void Disconnect() override
	{
		asio::error_code ec;
		SC::GetSocket().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		SC::GetSocket().close(ec);
		timeout_timer_.cancel(ec);
	}

	void OnResolve(const asio::error_code& ec, asio::ip::tcp::resolver::iterator iterator, std::shared_ptr<SocketCtx> ctx)
	{
		if (ctx->IsStopped())
			return;

		if (!ec)
		{
			StartConnection(iterator, ctx);
		}
		else
		{
			std::cout << "TCPSocketClient OnResolve error: " << ec.message() << std::endl;
			SC::DisconnectSocket();
		}
	}

	void StartConnection(asio::ip::tcp::resolver::iterator endpoint_iter, std::shared_ptr<SocketCtx>& ctx)
	{
		if (ctx->IsStopped())
			return;

		if (endpoint_iter != asio::ip::tcp::resolver::iterator())
		{
			std::cout << "Trying " << endpoint_iter->endpoint() << "...\n";

			if (timeout_)
			{
				asio::error_code ec;
				timeout_timer_.expires_from_now(std::chrono::seconds(timeout_), ec);
				timeout_timer_.async_wait(bind(&TCPSocketClient::TimerCallback,
					std::static_pointer_cast<TCPSocketClient>(SC::shared_from_this()),
					std::placeholders::_1, ctx));
			}

			SC::GetSocket().async_connect(endpoint_iter->endpoint(), std::bind(&TCPSocketClient::ConnectCallback,
				std::static_pointer_cast<TCPSocketClient>(SC::shared_from_this()),
				std::placeholders::_1, endpoint_iter, ctx));
		}
		else
		{
			// Connection failed
			SC::DisconnectSocket();
		}
	}

	void ConnectCallback(const asio::error_code& ec, asio::ip::tcp::resolver::iterator iterator, std::shared_ptr<SocketCtx> ctx)
	{
		if (ctx->IsStopped())
			return;

		// If the socket is closed it means that the connection timed-out
		// and the callback for the timer closed it
		if (!SC::GetSocket().is_open())
		{
			std::cout << "QMP connection timed out" << std::endl;
			StartConnection(++iterator, ctx);
		}
		else if (ec)
		{
			std::cout << "QMP connection failed: " << ec.message() << std::endl;
			asio::error_code ec;
			SC::GetSocket().close(ec);
			StartConnection(++iterator, ctx);
		}
		else
		{
			asio::error_code ec;
			timeout_timer_.cancel(ec);

			Base::OnConnect(ctx);
		}
	}

	void TimerCallback(const asio::error_code& ec, std::shared_ptr<SocketCtx> ctx)
	{
		if (ec || ctx->IsStopped())
			return;

		asio::error_code err;
		SC::GetSocket().close(err);
	}

	asio::ip::tcp::resolver resolver_;
	asio::steady_timer timeout_timer_;
	std::string host_;
	std::string port_;
	uint16_t timeout_;
};
