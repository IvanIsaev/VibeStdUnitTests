/*
 * =============================================================================
 * Monitor Object (Concurrency pattern — e.g. Schmidt, *Pattern-Oriented SW Arch.*)
 * =============================================================================
 *
 * What it is
 * ----------
 * A **monitor** bundles **shared mutable state** with a **lock** and, when
 * needed, **condition variables** so threads can **wait** until a predicate
 * holds. Every **public** method of the object follows the same protocol:
 * acquire the monitor lock, read/write fields, possibly `wait` / `notify`, then
 * release. External code never touches the fields without going through those
 * methods.
 *
 * This is the pattern behind Java’s **`synchronized` methods**, C# `lock (this)`
 * (discouraged style but same idea), and hand-written C++ services that guard
 * a `std::mutex` + `std::condition_variable` per instance.
 *
 * Structure
 * ---------
 *   • **Monitor lock** — typically one `std::mutex` (or `std::shared_mutex` for
 *     reader/writer variants) private to the object.
 *
 *   • **Synchronized methods** — `deposit`, `take`, `await_ready`: each takes
 *     `std::unique_lock` / `lock_guard` at entry; **all** invariants are
 *     maintained while the lock is held.
 *
 *   • **Condition queue(s)** — `std::condition_variable` (or several: `not_empty`,
 *     `not_full`, `phase_changed`) paired with **predicate loops** to handle
 *     spurious wakeups.
 *
 *   • **Monitor data** — the actual fields (queue, balance, flags).
 *
 * Waiting and signaling (Mesa vs Hoare)
 * --------------------------------------
 * C++ `condition_variable` uses **Mesa-style** semantics: `notify_one` marks
 * waiters **eligible** to run; they must **re-check** the predicate after waking
 * because another thread may steal the slot. Always write:
 *
 *   `cv.wait(lock, [&]{ return predicate; });`
 *
 * Never assume “notify means my condition is true forever.”
 *
 * Monitor vs Active Object
 * ------------------------
 *   • **Monitor** — caller **blocks inside** the method while holding (or
 *     waiting on) the lock; work runs **on the caller’s thread** (unless you
 *     offload internally).
 *
 *   • **Active Object** — caller **returns quickly**; work is **queued** and
 *     executed on a **dedicated** thread.
 *
 * Choose monitors when synchronous mutual exclusion is enough; choose active
 * objects when you need **serialization + async** behavior (UI thread, strand).
 *
 * Pitfalls
 * --------
 *   • **Deadlock** — `std::mutex` is **not** recursive: a method that calls
 *     another `synchronized` method on the **same** object without releasing
 *     the lock will **block forever**. Use **re-entrant** (`std::recursive_mutex`)
 *     sparingly, or **split** helpers that assume the lock is already held.
 *
 *   • **Lock ordering** — multiple monitors: always acquire in a **global** order
 *     to avoid AB-BA deadlock.
 *
 *   • **Holding locks while calling unknown code** — calling callbacks while
 *     locked risks reentrancy and long critical sections; document or copy state
 *     then unlock before notifying outsiders.
 *
 *   • **Lost wakeups** — ensure **predicate + notify** happen under the **same**
 *     mutex so a waiter cannot miss the signal.
 *
 * Related ideas
 * -------------
 *   • **std::scoped_lock** — lock multiple mutexes safely for transactions.
 *
 *   • **Monitors in the stdlib** — `std::queue` is **not** thread-safe; wrapping
 *     it in a monitor is the textbook fix.
 *
 *   • **Semaphores** — can implement bounded buffers too; monitors express
 *     **structured** invariants around a concrete object.
 *
 * Testing
 * -------
 *   • **Stress** — many threads, invariants (balance ≥ 0, queue size ≤ cap).
 *
 *   • **Blocking** — use short timeouts in tests or `notify` from a control
 *     thread so tests don’t hang on regressions.
 *
 *   • **One-definition rule** — types live in `monitor_object_demo` for this
 *     `usage_examples` binary.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace usage_examples::patterns::concurrency::monitor_object_demo {

	// -----------------------------------------------------------------
	// Example 1 — Bounded buffer monitor (not_full / not_empty)
	// -----------------------------------------------------------------
	class BoundedIntQueueMonitor
	{
	public:
		explicit BoundedIntQueueMonitor(std::size_t capacity) : capacity_(capacity)
		{
			if (capacity_ == 0) throw std::invalid_argument("BoundedIntQueueMonitor: capacity");
		}

		void put(int value)
		{
			std::unique_lock lk(mx_);
			not_full_.wait(lk, [this] { return queue_.size() < capacity_; });
			queue_.push_back(value);
			not_empty_.notify_one();
		}

		int take()
		{
			std::unique_lock lk(mx_);
			not_empty_.wait(lk, [this] { return !queue_.empty(); });
			const int v = queue_.front();
			queue_.pop_front();
			not_full_.notify_one();
			return v;
		}

	private:
		std::mutex mx_{};
		std::condition_variable not_full_{};
		std::condition_variable not_empty_{};
		std::deque<int> queue_{};
		std::size_t capacity_{};
	};

	// -----------------------------------------------------------------
	// Example 2 — Account: every public method is synchronized on one mutex
	// -----------------------------------------------------------------
	class GuardedAccountMonitor
	{
	public:
		explicit GuardedAccountMonitor(int opening_balance_cents) : balance_cents_(opening_balance_cents)
		{
			if (balance_cents_ < 0) throw std::invalid_argument("GuardedAccountMonitor: negative opening");
		}

		void deposit(int cents)
		{
			if (cents < 0) throw std::invalid_argument("GuardedAccountMonitor: deposit");
			std::lock_guard lk(mx_);
			balance_cents_ += cents;
		}

		void withdraw(int cents)
		{
			if (cents < 0) throw std::invalid_argument("GuardedAccountMonitor: withdraw");
			std::lock_guard lk(mx_);
			if (cents > balance_cents_) throw std::runtime_error("insufficient funds");
			balance_cents_ -= cents;
		}

		[[nodiscard]] int balance_cents() const
		{
			std::lock_guard lk(mx_);
			return balance_cents_;
		}

	private:
		mutable std::mutex mx_{};
		int balance_cents_{};
	};

	// -----------------------------------------------------------------
	// Example 3 — Gate: threads wait until opened (single condition)
	// -----------------------------------------------------------------
	class OpenGateMonitor
	{
	public:
		void wait_until_open()
		{
			std::unique_lock lk(mx_);
			opened_.wait(lk, [this] { return is_open_; });
		}

		template <class Rep, class Period>
		bool wait_until_open_for(const std::chrono::duration<Rep, Period>& rel_time)
		{
			std::unique_lock lk(mx_);
			return opened_.wait_for(lk, rel_time, [this] { return is_open_; });
		}

		void open()
		{
			{
				std::lock_guard lk(mx_);
				is_open_ = true;
			}
			opened_.notify_all();
		}

		[[nodiscard]] bool is_open() const
		{
			std::lock_guard lk(mx_);
			return is_open_;
		}

	private:
		mutable std::mutex mx_{};
		std::condition_variable opened_{};
		bool is_open_{false};
	};

} // namespace usage_examples::patterns::concurrency::monitor_object_demo

namespace {

	using usage_examples::patterns::concurrency::monitor_object_demo::BoundedIntQueueMonitor;
	using usage_examples::patterns::concurrency::monitor_object_demo::GuardedAccountMonitor;
	using usage_examples::patterns::concurrency::monitor_object_demo::OpenGateMonitor;

	TEST(MonitorObjectUsageExamples, BoundedQueuePreservesFifoOrder)
	{
		BoundedIntQueueMonitor q(4);
		q.put(10);
		q.put(20);
		q.put(30);
		EXPECT_EQ(q.take(), 10);
		EXPECT_EQ(q.take(), 20);
		EXPECT_EQ(q.take(), 30);
	}

	TEST(MonitorObjectUsageExamples, BoundedQueueBlocksProducersWhenFull)
	{
		BoundedIntQueueMonitor q(2);
		q.put(1);
		q.put(2);
		bool third_put_done = false;
		std::thread producer([&] {
			q.put(3);
			third_put_done = true;
		});
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		EXPECT_FALSE(third_put_done);
		EXPECT_EQ(q.take(), 1);
		producer.join();
		EXPECT_TRUE(third_put_done);
		EXPECT_EQ(q.take(), 2);
		EXPECT_EQ(q.take(), 3);
	}

	TEST(MonitorObjectUsageExamples, GuardedAccountSurvivesConcurrentDeposits)
	{
		GuardedAccountMonitor acct(0);
		constexpr int kThreads = 10;
		constexpr int kEach = 500;
		std::vector<std::thread> threads;
		threads.reserve(kThreads);
		for (int i = 0; i < kThreads; ++i)
		{
			(void)i;
			threads.emplace_back([&acct] {
				for (int i = 0; i < kEach; ++i) acct.deposit(1);
			});
		}
		for (auto& th : threads) th.join();
		EXPECT_EQ(acct.balance_cents(), kThreads * kEach);
	}

	TEST(MonitorObjectUsageExamples, OpenGateReleasesWaiters)
	{
		OpenGateMonitor gate;
		int stage = 0;
		std::thread waiter([&] {
			gate.wait_until_open();
			stage = 1;
		});
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		EXPECT_EQ(stage, 0);
		gate.open();
		waiter.join();
		EXPECT_EQ(stage, 1);
		EXPECT_TRUE(gate.is_open());
	}

	TEST(MonitorObjectUsageExamples, WaitForReturnsFalseWhenTimeout)
	{
		OpenGateMonitor gate;
		EXPECT_FALSE(gate.wait_until_open_for(std::chrono::milliseconds(5)));
		gate.open();
		EXPECT_TRUE(gate.wait_until_open_for(std::chrono::milliseconds(50)));
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add **try_put** / **try_take** with `std::chrono::milliseconds` timeouts and
 *    `wait_for` predicates.
 * 2. Implement a **reader-preferring** or **writer-preferring** `shared_mutex`
 *    monitor around a large read-mostly cache.
 * 3. Compare **monitor** throughput to a **lock-free** MPMC queue for the same
 *    bounded-buffer workload (measure p99 latency).
 * 4. Document **lock ordering** when `GuardedAccountMonitor::transfer_to` must
 *    lock two accounts without deadlock.
 */
