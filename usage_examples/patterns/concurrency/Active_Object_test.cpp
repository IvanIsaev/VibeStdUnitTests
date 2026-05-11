/*
 * =============================================================================
 * Active Object (Concurrency pattern — e.g. Schmidt, *Pattern-Oriented SW Arch.*)
 * =============================================================================
 *
 * What it is
 * ----------
 * The **Active Object** pattern decouples **method invocation** (on the calling
 * thread) from **method execution** (on a dedicated thread). Clients talk to a
 * lightweight **Proxy**; work is converted into **Method Requests**, queued in
 * an **Activation Queue**, and a **Scheduler** dispatches them to a **Servant**
 * that owns the real state—**without** clients locking that state.
 *
 * Think “one actor, one mailbox”: UI message pumps, game simulation ticks,
 * single-writer services, or reactor-style workers that must serialize access to
 * a subsystem.
 *
 * Classic structure (POSA2-style)
 * --------------------------------
 *   • **Proxy** — client-visible façade (`enqueue`, `post`, `async_foo`).
 *
 *   • **Method request** — command object / `std::function` / `packaged_task`
 *     carrying the call to run later.
 *
 *   • **Activation queue** — bounded or unbounded buffer between callers and
 *     scheduler (mutex + `condition_variable`, lock-free MPMC, `asio::strand`, …).
 *
 *   • **Scheduler** — loop that dequeues and executes; may prioritize, batch,
 *     or time-slice.
 *
 *   • **Servant** — domain logic; should see **single-threaded** access if all
 *     mutations go through the scheduler (no extra locks needed on the servant).
 *
 *   • **Future** (optional) — return values / errors via `std::future`,
 *     callbacks, or coroutine awaiters. `std::packaged_task` is **move-only**;
 *     if you enqueue into `std::function<void()>`, wrap the task in
 *     `std::shared_ptr` so the closure stays **copyable** as the standard requires.
 *
 * Why use it
 * ----------
 *   • **Deterministic ordering** — all mutations pass through one queue (easy
 *     reasoning; natural match for **event sourcing** per aggregate).
 *
 *   • **Caller never blocks long** — heavy work runs elsewhere (unless the
 *     queue is full and you **back-pressure**).
 *
 *   • **Composability** — multiple proxies can feed one scheduler, or pipelines
 *     chain active objects.
 *
 * Trade-offs
 * ----------
 *   • **Latency** — every call pays **queue + context-switch** overhead; unsuitable
 *     for nanosecond hot paths without batching.
 *
 *   • **Throughput limits** — one servant thread caps work; scale with **sharding**
 *     (partitioned active objects) or **work stealing**.
 *
 *   • **Shutdown** — must **drain** the queue, cancel timers, and define what
 *     happens to blocked `future::get()` callers. In C++, declare the **scheduler
 *     after** the **servant** so the worker joins (or the queue drains) **before**
 *     the servant’s storage is destroyed (reverse member destruction order).
 *
 *   • **Reentrancy** — avoid servant code that synchronously calls back into the
 *     same active object and **deadlocks** (same thread waits on itself). Use
 *     **async** replies or separate queues.
 *
 * Related ideas
 * -------------
 *   • **Actor model** — mailbox + behavior; often many active objects.
 *
 *   • **Reactor / Proactor** — demultiplex I/O; active object often **consumes**
 *     events from a reactor thread.
 *
 *   • **Thread pools** — general parallel execution; active object is the
 *     **special case** “pool of size 1 with strict ordering.”
 *
 *   • **`asio::strand` / `dispatch` / Android `Handler`** — platform idioms for
 *     the same idea.
 *
 * Testing
 * -------
 *   • **Ordering** — enqueue A then B; observe servant state `AB`.
 *
 *   • **Concurrent producers** — many threads post; final state matches a
 *     **serialized** replay if the servant is deterministic.
 *
 *   • **Shutdown** — destructor / `stop()` leaves no leaked threads; pending work
 *     policy is documented (drain vs drop).
 *
 *   • **One-definition rule** — types live in `active_object_demo` for this
 *     `usage_examples` binary.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace usage_examples::patterns::concurrency::active_object_demo {

	// -----------------------------------------------------------------
	// Activation engine — single worker, FIFO queue, drain on shutdown
	// -----------------------------------------------------------------
	class ActivationEngine
	{
	public:
		ActivationEngine() : worker_([this] { run(); }) {}

		~ActivationEngine()
		{
			{
				std::lock_guard lk(mx_);
				stopped_ = true;
			}
			cv_.notify_all();
			if (worker_.joinable()) worker_.join();
		}

		ActivationEngine(const ActivationEngine&) = delete;
		ActivationEngine& operator=(const ActivationEngine&) = delete;

		void dispatch(std::function<void()> job)
		{
			{
				std::lock_guard lk(mx_);
				if (stopped_) throw std::logic_error("ActivationEngine: dispatch after stop");
				tasks_.push(std::move(job));
			}
			cv_.notify_one();
		}

	private:
		void run()
		{
			for (;;)
			{
				std::function<void()> job;
				{
					std::unique_lock lk(mx_);
					cv_.wait(lk, [this] { return stopped_ || !tasks_.empty(); });
					if (stopped_ && tasks_.empty()) return;
					job = std::move(tasks_.front());
					tasks_.pop();
				}
				job();
			}
		}

		std::mutex mx_{};
		std::condition_variable cv_{};
		std::queue<std::function<void()>> tasks_{};
		bool stopped_{false};
		std::thread worker_{};
	};

	// -----------------------------------------------------------------
	// Example 1 — Servant: append-only string log (single-threaded mutations)
	// -----------------------------------------------------------------
	class ActiveAppendLog
	{
	public:
		void append(std::string_view chunk)
		{
			std::string copy(chunk);
			engine_.dispatch([this, s = std::move(copy)]() mutable { servant_.buffer += s; });
		}

		[[nodiscard]] std::future<std::string> snapshot_async() const
		{
			auto task = std::make_shared<std::packaged_task<std::string()>>([this] { return servant_.buffer; });
			std::future<std::string> fut = task->get_future();
			engine_.dispatch([task]() { (*task)(); });
			return fut;
		}

	private:
		struct Servant
		{
			std::string buffer{};
		};

		// Engine last: destructor joins worker before `servant_` is torn down.
		mutable Servant servant_{};
		mutable ActivationEngine engine_{};
	};

	// -----------------------------------------------------------------
	// Example 2 — Wallet: deposits are async; balance via future
	// -----------------------------------------------------------------
	class ActiveWallet
	{
	public:
		void deposit(int cents)
		{
			engine_.dispatch([this, cents] {
				if (cents < 0) return;
				servant_.balance += cents;
			});
		}

		void withdraw(int cents)
		{
			engine_.dispatch([this, cents] {
				if (cents < 0) return;
				servant_.balance -= cents;
				if (servant_.balance < 0) servant_.balance = 0;
			});
		}

		[[nodiscard]] std::future<int> balance_async() const
		{
			auto task = std::make_shared<std::packaged_task<int()>>([this] { return servant_.balance; });
			std::future<int> fut = task->get_future();
			engine_.dispatch([task]() { (*task)(); });
			return fut;
		}

	private:
		struct Servant
		{
			int balance{0};
		};

		mutable Servant servant_{};
		mutable ActivationEngine engine_{};
	};

	// -----------------------------------------------------------------
	// Example 3 — “Method request” counter: batch N increments in one enqueue
	// -----------------------------------------------------------------
	class ActiveBatchCounter
	{
	public:
		void add_many(int count, int delta)
		{
			engine_.dispatch([this, count, delta] {
				for (int i = 0; i < count; ++i) servant_.value += delta;
			});
		}

		[[nodiscard]] std::future<long long> value_async() const
		{
			auto task = std::make_shared<std::packaged_task<long long()>>([this] { return servant_.value; });
			std::future<long long> fut = task->get_future();
			engine_.dispatch([task]() { (*task)(); });
			return fut;
		}

	private:
		struct Servant
		{
			long long value{0};
		};

		mutable Servant servant_{};
		mutable ActivationEngine engine_{};
	};

} // namespace usage_examples::patterns::concurrency::active_object_demo

namespace {

	using usage_examples::patterns::concurrency::active_object_demo::ActiveAppendLog;
	using usage_examples::patterns::concurrency::active_object_demo::ActiveBatchCounter;
	using usage_examples::patterns::concurrency::active_object_demo::ActiveWallet;

	TEST(ActiveObjectUsageExamples, AppendLogPreservesOrderForSequentialPosts)
	{
		ActiveAppendLog log;
		log.append("hel");
		log.append("lo");
		auto fut = log.snapshot_async();
		ASSERT_EQ(fut.wait_for(std::chrono::seconds(1)), std::future_status::ready);
		EXPECT_EQ(fut.get(), "hello");
	}

	TEST(ActiveObjectUsageExamples, WalletDepositsAndReadsViaFuture)
	{
		ActiveWallet wallet;
		wallet.deposit(100);
		wallet.deposit(50);
		wallet.withdraw(30);
		auto fut = wallet.balance_async();
		ASSERT_EQ(fut.wait_for(std::chrono::seconds(1)), std::future_status::ready);
		EXPECT_EQ(fut.get(), 120);
	}

	TEST(ActiveObjectUsageExamples, ConcurrentProducersEventuallyDrainToServant)
	{
		ActiveAppendLog log;
		constexpr int kThreads = 6;
		constexpr int kEach = 200;
		std::vector<std::thread> threads;
		threads.reserve(kThreads);

		for (int t = 0; t < kThreads; ++t)
		{
			threads.emplace_back([&log] {
				for (int i = 0; i < kEach; ++i) log.append("x");
			});
		}

		for (auto& th : threads) th.join();

		auto fut = log.snapshot_async();
		ASSERT_EQ(fut.wait_for(std::chrono::seconds(2)), std::future_status::ready);
		const std::string blob = fut.get();
		EXPECT_EQ(blob.size(), static_cast<unsigned>(kThreads * kEach));
	}

	TEST(ActiveObjectUsageExamples, BatchedWorkRunsOnServantThread)
	{
		ActiveBatchCounter ctr;
		ctr.add_many(10'000, 3);
		auto fut = ctr.value_async();
		ASSERT_EQ(fut.wait_for(std::chrono::seconds(1)), std::future_status::ready);
		EXPECT_EQ(fut.get(), 30'000LL);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add a **bounded** queue with `try_dispatch` / back-pressure when full.
 * 2. Replace `std::function` with typed **Method Request** structs + visitor for
 *    auditing and priority lanes.
 * 3. Integrate **`std::jthread`** + `std::stop_token` for cooperative cancel and
 *    timeout draining.
 * 4. Compare latency with a **mutex-protected** servant updated directly from
 *    many threads — profile queue + single worker vs lock contention.
 */
