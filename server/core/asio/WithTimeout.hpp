//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// SPDX-License-Identifier: GPL-3.0
//

#pragma once

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <core/asio/AsioConfig.hpp>

namespace collab3::core::detail {

	template <typename CompletionToken>
	struct TimedToken {
		SteadyTimerType& timer;
		CompletionToken& token;
	};

	template <typename... Signatures>
	struct TimedInitiation {
		template <typename CompletionHandler, typename Initiation, typename... InitArgs>
		void operator()(CompletionHandler handler, SteadyTimerType& timer, Initiation&& initiation, InitArgs&&... init_args) {
			using net::experimental::make_parallel_group;

			auto ex = net::get_associated_executor(handler, net::get_associated_executor(initiation));

			// clang-format off
			make_parallel_group(
				net::bind_executor(ex, [&](auto&& token) {
					return timer.async_wait(std::forward<decltype(token)>(token));
				}),
				net::bind_executor(ex, [&](auto&& token) {
					// user's underlying operation.
					return net::async_initiate<decltype(token), Signatures...>(std::forward<Initiation>(initiation), token, std::forward<InitArgs>(init_args)...);
				})
			).async_wait(net::experimental::wait_for_one(),
				[handler = std::move(handler)](std::array<std::size_t, 2>, std::error_code, auto... underlying_op_results) mutable {
					std::move(handler)(std::move(underlying_op_results)...);
			});
			// clang-format on
		}
	};
} // namespace ls::detail

namespace boost::asio {
	template <typename InnerCompletionToken, typename... Signatures>
	struct async_result<collab3::core::detail::TimedToken<InnerCompletionToken>, Signatures...> {
		template <typename Initiation, typename... InitArgs>
		static auto initiate(Initiation&& init, collab3::core::detail::TimedToken<InnerCompletionToken> t, InitArgs&&... init_args) {
			return asio::async_initiate<InnerCompletionToken, Signatures...>(collab3::core::detail::TimedInitiation<Signatures...> {}, t.token, t.timer, std::forward<Initiation>(init), std::forward<InitArgs>(init_args)...);
		}
	};
} // namespace boost::asio

namespace collab3::core {
	template <typename CompletionToken>
	detail::TimedToken<CompletionToken> Timed(SteadyTimerType& timer, CompletionToken&& token) {
		return detail::TimedToken<CompletionToken> { timer, token };
	}

	/**
	 * Call a ASIO operation with a timeout.
	 * The timer is supplied by the user, and must last until the completion token is called.
	 *
	 * \param[in] op The asynchronous operation to wrap with a timeout (use net::deferred as the CompletionToken)
	 * \param[in] timer The timer to use as the deadline timer. Must be configured with a expiry beforehand
	 * \param[in] token The completion token for this operation.
	 */
	template <typename Op, typename CompletionToken>
	auto WithTimeout(Op op, SteadyTimerType& timer, CompletionToken&& token) {
		return std::move(op)(Timed(timer, std::forward<CompletionToken>(token)));
	}
} // namespace ls