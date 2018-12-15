#pragma once
#include <boost/asio.hpp>

/**
 * A simple structure containing a boolean that indicates
 * when the socket has disconnected. This is used to prevent
 * the OnDisconnect callback from being called multiple times
 * when there are multiple async callbacks that fail on the socket.
 */
struct SocketCtx
{
	/**
	 * Indicates if the socket has been disconnected.
	 * This should be checked by the socket implementations
	 * before performing any read or write operations on the socket.
	 */
	bool IsStopped() const
	{
		return stopped_;
	}

	// Should only be accessed by SocketClient
	bool stopped_ = false;
};

/**
 * The SocketClient class allows clients to use
 * different types of ASIO sockets even though they do
 * not derive from a common base class.
 *
 * The base type implements the protocol functionality
 * for reading and writing using the ASIO socket.
 * Base Type Requirements:
 *
 * void OnConnect(std::shared_ptr<SocketCtx>& ctx);
 * Called when the socket has connected from within the
 * io_service loop.
 *
 * void OnDisconnect();
 * Called when the socket has disconnected from within the
 * io_service loop.
 *
 * std::shared_ptr<Base> shared_from_this();
 * Provides a shared_ptr to the Base class that can
 * be used to prevent the class from being destructed
 * during an async operation. The base class should
 * derive from std::enabled_shared_from_this to
 * implement this function.
 *
 */
template<typename Base, typename SocketType>
class SocketClient : public Base
{
public:
	/**
	 * @param service An ASIO service object that must be associated with only one thread.
	 */
	SocketClient(boost::asio::io_service& service) :
		Base(service),
		service_(service),
		socket_(service)
	{
	}

	void ConnectSocket() override
	{
		auto self = std::static_pointer_cast<SocketClient>(Base::shared_from_this());
		service_.dispatch([this, self]()
		{
			if (!socket_ctx_)
			{
				socket_ctx_ = std::make_shared<SocketCtx>();
				ConnectSocket(socket_ctx_);
			}
		});
	}

	void DisconnectSocket() override
	{
		auto self = std::static_pointer_cast<SocketClient>(Base::shared_from_this());
		service_.dispatch([self, this]()
		{
			if (socket_ctx_)
			{
				socket_ctx_->stopped_ = true;
				socket_ctx_.reset();
				Disconnect();
				Base::OnDisconnect();
			}
		});
	}

	boost::asio::io_service& GetService() override
	{
		return service_;
	}

	SocketType& GetSocket()
	{
		return socket_;
	}

	std::shared_ptr<SocketCtx>& GetSocketContext() override
	{
		return socket_ctx_;
	}

protected:
	virtual void ConnectSocket(std::shared_ptr<SocketCtx>& ctx) = 0;
	virtual void Disconnect() = 0;

private:
	boost::asio::io_service& service_;
	/**
	 * For thread safety, the socket and socket context should only
	 * be accessed from within a single thread that is apart of the io_service.
	 */
	SocketType socket_;
	std::shared_ptr<SocketCtx> socket_ctx_;
};
