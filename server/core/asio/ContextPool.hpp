//
// Created by lily on 9/19/22.
//

#ifndef COLLAB3_CONTEXTPOOL_HPP
#define COLLAB3_CONTEXTPOOL_HPP

#include <core/asio/AsioConfig.hpp>
#include <boost/asio/io_context.hpp>

#include <memory>
#include <thread>
#include <optional>


namespace collab3::core {

	/**
	 * A pool of ASIO io_contexts with a parent thread for each of them.
	 * Models io_context per thread model implicitly.
	 */
	struct ContextPool {

		inline ContextPool()
			: ContextPool(std::thread::hardware_concurrency() / 2) {

		}

		ContextPool(std::uint64_t nrThreads);

		/**
		 * Join all threads.
		 */
		void Join();

		/**
		 * Start all threads
		 */
		void Start();

		/**
		 * Cancel all threads.
		 */
		void Stop();

		/**
		 * Get a context in a round robin fashion.
		 */
		net::io_context& GetContext() const noexcept;

	   private:
		struct ThreadAndIOContext {

			void Start();

			// Cancel this thread.
			void Cancel();

			void Join();

			net::io_context& GetContext() const noexcept;

		   private:
			/**
			 * Worker function.
			 */
			void ThreadWorker();

			std::optional<std::thread> thread;
			net::io_context ioc;
		};

		//using
	};

}

#endif //COLLAB3_CONTEXTPOOL_HPP
