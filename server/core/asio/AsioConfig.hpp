#ifndef COLLAB3_CORE_ASIOCONFIG_H
#define COLLAB3_CORE_ASIOCONFIG_H

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
	namespace ip = net::ip;
	using tcp = ip::tcp;

	/**
	 * The type for executors to follow.
	 * This is the basic executor type.
	 */
	using BaseExecutorType =
#ifdef COLLAB3_CORE_SYSTEM_EXECUTOR
		net::strand<net::system_executor>;
#else
		net::strand<net::io_context::executor_type>;
#endif

	/**
	 * The (real) type executors follow. This is configured
	 * to by-default allow asynchronous operations to be
	 * - tuple returns (use auto [ ec, ... ] to nab at values; also no exceptions)
	 * - awaitable (use co_await to await)
	 */
	using ExecutorType =
		net::as_tuple_t<net::use_awaitable_t<BaseExecutorType>>::executor_with_default<BaseExecutorType>;

	/**
	 * Awaitable type (configured for the current collab3 executor)
	 */
	template<class T>
	using Awaitable = net::awaitable<T, ExecutorType>;

	template<typename Protocol>
	using SocketType = net::basic_stream_socket<Protocol, ExecutorType>;

	using TcpSocketType = SocketType<tcp>;
	using UnixStreamSocketType = SocketType<net::local::stream_protocol>;

	using TimerType = net::basic_waitable_timer<std::chrono::steady_clock, net::wait_traits<std::chrono::steady_clock>, ExecutorType>;

	// NB: This stuff should go to a different header, probably.

	namespace beast = boost::beast;

	template<typename Protocol>
	using StreamType = beast::basic_stream<Protocol, ExecutorType>;

	using TcpStreamType = StreamType<tcp>;
	using UnixStreamStreamType = StreamType<net::local::stream_protocol>;

}

#endif //COLLAB3_CORE_ASIOCONFIG_H
