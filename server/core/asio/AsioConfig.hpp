#pragma once

#include <boost/beast/core/basic_stream.hpp>
#include <boost/beast/core/error.hpp>

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/system_executor.hpp>

namespace collab3::core {

	namespace net = boost::asio;
	using tcp = net::ip::tcp;
	using unix_stream = net::local::stream_protocol;

	using error_code = boost::system::error_code;

	/**
	 * The type for executors to follow.
	 * This is the basic executor type.
	 */
	using BaseExecutorType = net::system_executor;

	/**
	 * The (real) type executors follow.
	 */
	using ExecutorType = net::strand<BaseExecutorType>;

	/**
	 * CompletionToken for allowing usage of coroutines w/o exception handling.
	 * \see Boost.Asio CompletionToken
	 */
	constexpr inline auto use_tuple_awaitable = net::as_tuple(net::use_awaitable_t<ExecutorType> {});

	/**
	 * Awaitable type (configured for the current executor)
	 */
	template <class T>
	using Awaitable = net::awaitable<T, ExecutorType>;

	template <typename Protocol>
	using AcceptorType = net::basic_socket_acceptor<Protocol, ExecutorType>;

	template <typename Protocol>
	using SocketType = net::basic_stream_socket<Protocol, ExecutorType>;

	using SteadyTimerType = net::basic_waitable_timer<std::chrono::steady_clock, net::wait_traits<std::chrono::steady_clock>, ExecutorType>;


	namespace beast = boost::beast;

	template<typename Protocol>
	using StreamType = beast::basic_stream<Protocol, ExecutorType>;

	using TcpStreamType = StreamType<tcp>;
	using UnixStreamStreamType = StreamType<net::local::stream_protocol>;

	/**
	 * Make an instance of the configured Asio executor type.
	 */
	inline ExecutorType MakeExecutor() {
		return net::make_strand(
			BaseExecutorType{}
		);
	}
}
