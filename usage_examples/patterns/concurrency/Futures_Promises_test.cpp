/*
 * =============================================================================
 * Futures & Promises (Concurrency idiom — C++ standard library)
 * =============================================================================
 *
 * What it is
 * ----------
 * A **Future** is a **handle to a result** that may not be ready yet: the
 * consumer can **wait**, **poll with a timeout**, or **chain** work (manually or
 * via executors). A **Promise** is the **producer side** of that contract: one
 * place in the program **fulfills** the result (value, exception, or
 * cancellation signal).
 *
 * In ISO C++, the core vocabulary types are:
 *
 *   • **`std::promise<T>`** — create a paired **`std::future<T>`**; call
 *     `set_value`, `set_exception`, or let destruction publish an error state.
 *
 *   • **`std::future<T>`** — **single consumer**; `get()` moves the result out
 *     (future becomes invalid afterward for `T` that is not void—`void` futures
 *     still become invalid after `get()`).
 *
 *   • **`std::shared_future<T>`** — **multiple** waiters can read the same
 *     outcome (copyable handle; `get()` returns `const T&` for non-void).
 *
 *   • **`std::packaged_task<R(Args...)>`** — wraps a **callable** so invoking it
 *     fulfills a future (used by thread pools and `std::async` under the hood).
 *
 * Why use it
 * ----------
 *   • **Decouple** “start work” from “need result now” — producers signal
 *     completion without callbacks wired through every layer.
 *
 *   • **Standard exception channel** — `set_exception` / uncaught exceptions in
 *     `packaged_task` become **`future::get()`** rethrows.
 *
 *   • **Composability** — combine with **thread pools**, **executors**, or
 *     **coroutines** (`co_await` on awaitable futures in C++20+ ecosystems).
 *
 * `std::async` (use carefully)
 * -----------------------------
 * `std::async(policy, f, args...)` returns `std::future` but **how** `f` runs
 * depends on **policy** and the **implementation** (`async` may spawn a thread,
 * use a thread pool, or defer until `wait/get`). Treat it as a **portable
 * hint**, not a hard real-time guarantee. For predictable threading, prefer an
 * explicit **`std::thread`**, **pool**, or **task framework**.
 *
 * Waiting and timeouts
 * --------------------
 *   • `wait()` — block until ready.
 *
 *   • `wait_for` / `wait_until` — return **`future_status::timeout`** if not
 *     ready; useful for **UI loops** and **shutdown**.
 *
 *   • **`shared_future::wait`** — multiple threads can wait; all see the same
 *     terminal state once ready.
 *
 * Lifetime and errors
 * -------------------
 *   • **Broken promise** — if the `std::promise` is **destroyed** without a
 *     value or exception, the paired `future` completes with **`future_error`**
 *     (`broken_promise`).
 *
 *   • **Double `set_value`** — second fulfillment on a promise is an error
 *     (`promise_already_satisfied`)—typically a logic bug.
 *
 *   • **Invalid future** — default-constructed or moved-from `future`; `valid()`
 *     is false; `get` is undefined—check `valid()` in generic utilities.
 *
 * Related patterns
 * ----------------
 *   • **Thread Pool** — `submit` often returns `future` backed by
 *     `packaged_task`.
 *
 *   • **Active Object** — method requests can surface results as futures.
 *
 *   • **Reactive / callback** styles — futures are **pull**; callbacks are
 *     **push**; bridges exist (`then` in libraries, coroutines).
 *
 * Testing
 * -------
 *   • **Value / exception** paths; **timeout** paths; **broken promise**.
 *
 *   • **Sanitizers** when futures cross threads (data races on captured state).
 *
 *   • **One-definition rule** — helpers live in `futures_promises_demo` for this
 *     `usage_examples` binary.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace usage_examples::patterns::concurrency::futures_promises_demo {

	inline int slow_add(int a, int b)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		return a + b;
	}

} // namespace usage_examples::patterns::concurrency::futures_promises_demo

namespace {

	using usage_examples::patterns::concurrency::futures_promises_demo::slow_add;

	TEST(FuturesPromisesUsageExamples, PromiseFulfillsFutureWithValue)
	{
		std::promise<int> prom;
		std::future<int> fut = prom.get_future();
		std::thread producer([&prom] { prom.set_value(99); });
		EXPECT_EQ(fut.get(), 99);
		producer.join();
	}

	TEST(FuturesPromisesUsageExamples, SetExceptionPropagatesOnGet)
	{
		std::promise<std::string> prom;
		std::future<std::string> fut = prom.get_future();
		std::thread producer([&prom] {
			try { throw std::runtime_error("no string"); }
			catch (...)
			{
				prom.set_exception(std::current_exception());
			}
		});
		EXPECT_THROW(fut.get(), std::runtime_error);
		producer.join();
	}

	TEST(FuturesPromisesUsageExamples, BrokenPromiseWhenProducerDiesEarly)
	{
		std::future<int> fut;
		{
			std::promise<int> prom;
			fut = prom.get_future();
		}
		try
		{
			(void)fut.get();
			FAIL() << "expected future_error";
		}
		catch (const std::future_error& e)
		{
			EXPECT_EQ(e.code(), std::future_errc::broken_promise);
		}
	}

	TEST(FuturesPromisesUsageExamples, PackagedTaskRunsCallableAndSetsFuture)
	{
		std::packaged_task<int(int, int)> task(slow_add);
		std::future<int> fut = task.get_future();
		std::thread worker([t = std::move(task)]() mutable { t(2, 3); });
		EXPECT_EQ(fut.get(), 5);
		worker.join();
	}

	TEST(FuturesPromisesUsageExamples, SharedFutureAllowsMultipleWaiters)
	{
		std::promise<int> prom;
		std::shared_future<int> shared = prom.get_future().share();
		std::vector<std::thread> threads;
		std::atomic<int> sum{0};
		for (int i = 0; i < 4; ++i)
		{
			threads.emplace_back([&shared, &sum] {
				const int v = shared.get();
				sum.fetch_add(v, std::memory_order_relaxed);
			});
		}
		prom.set_value(7);
		for (auto& t : threads) t.join();
		EXPECT_EQ(sum.load(), 28);
	}

	TEST(FuturesPromisesUsageExamples, WaitForDetectsTimeout)
	{
		std::promise<void> prom;
		std::future<void> fut = prom.get_future();
		const auto status = fut.wait_for(std::chrono::milliseconds(10));
		EXPECT_EQ(status, std::future_status::timeout);
		prom.set_value();
		EXPECT_EQ(fut.wait_for(std::chrono::seconds(1)), std::future_status::ready);
	}

	TEST(FuturesPromisesUsageExamples, AsyncLaunchAsyncRunsFunction)
	{
		std::future<int> fut = std::async(std::launch::async, [] { return 11 * 6; });
		ASSERT_EQ(fut.wait_for(std::chrono::seconds(2)), std::future_status::ready);
		EXPECT_EQ(fut.get(), 66);
	}

	TEST(FuturesPromisesUsageExamples, DeferredRunsAtWaitOrGet)
	{
		std::atomic<bool> started{false};
		std::future<int> fut = std::async(std::launch::deferred, [&started] {
			started.store(true, std::memory_order_relaxed);
			return 3;
		});
		EXPECT_FALSE(started.load());
		EXPECT_EQ(fut.get(), 3);
		EXPECT_TRUE(started.load());
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Build **`when_all` / `when_any`** for `std::vector<std::future<T>>` with
 *    careful invalidation rules.
 * 2. Wrap a **callback-based** API (`void query(Callback)`) into a **`future`**.
 * 3. Compare **allocation** of `std::promise` per request vs **ring buffer** of
 *    pre-created promise slots for ultra-low latency RPC.
 * 4. Integrate with **C++20 coroutines** (`co_await` on a future-like type with
 *    `await_ready` / `await_suspend` / `await_resume`).
 */
