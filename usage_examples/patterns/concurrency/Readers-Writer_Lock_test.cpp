/*
 * =============================================================================
 * Readers–Writer Lock (Concurrency pattern)
 * =============================================================================
 *
 * What it is
 * ----------
 * A **readers–writer lock** (shared lock, multi-reader / single-writer lock)
 * lets **many threads read** shared data **at the same time** while **writes**
 * are **exclusive**. When reads dominate and are **cheap** compared to copying a
 * whole snapshot, this can beat a **plain mutex** that serializes everyone.
 *
 * In C++17 and later, the standard vocabulary is:
 *
 *   • **`std::shared_mutex`** — the lock object.
 *
 *   • **`std::shared_lock`** — **shared** (reader) mode: many concurrent holders.
 *
 *   • **`std::unique_lock`** — **exclusive** (writer) mode: one holder; blocks
 *     readers and other writers.
 *
 * Typical usage (sketch)
 * ----------------------
 * Reader path — many threads may enter at once:
 *
 *     void Config::for_each_route(std::function<void(const Route&)> fn) const {
 *       std::shared_lock lk(rw_);
 *       for (const auto& r : routes_) fn(r);
 *     }
 *
 * Writer path — excludes all readers and other writers:
 *
 *     void Config::replace_routes(std::vector<Route> next) {
 *       std::unique_lock lk(rw_);
 *       routes_ = std::move(next);
 *     }
 *
 * Keep **both** branches **short**: no blocking calls, no unknown callbacks while
 * holding the lock unless you accept stalled peers.
 *
 * Contrast with a single mutex
 * -----------------------------
 * With **`std::mutex`**, every `for_each_route` would **serialize** even though
 * readers only read. With **`std::shared_mutex`**, concurrent readers **overlap**
 * while writers still get **exclusive** access to the protected fields.
 *
 * Manual implementation (mental model)
 * ------------------------------------
 * Libraries often implement RW locks with **`std::mutex` + counters + one or
 * more `std::condition_variable`s**. A **reader-preference** policy might:
 * increment `readers` under the mutex, `notify_all` when the last reader leaves;
 * a **writer** waits until `readers == 0`. A **writer-preference** policy adds
 * a **`waiting_writers`** flag so new readers **defer** while a writer queues.
 * **`std::shared_mutex`** hides this; behavior is **implementation-defined**
 * regarding **fairness**.
 *
 * Common pitfalls
 * ---------------
 *   • **Lock ordering** — if you take `shared_mutex` **A** then **B** in one path
 *     and **B** then **A** in another → **deadlock**. Pick a global order.
 *
 *   • **Recursive / re-entrant** — `std::shared_mutex` is **not** recursive;
 *     double-locking the same thread → **undefined behavior**.
 *
 *   • **Const correctness** — readers use **`mutable std::shared_mutex rw_`** so
 *     `const` methods can still take `shared_lock`.
 *
 *   • **Protect everything** — every read/write of shared fields must go through
 *     the **same** lock object; mixing atomics and RW locks on the same logical
 *     datum without a **documented** memory model is a recipe for bugs.
 *
 * Databases and MVCC (context)
 * ----------------------------
 * **Multi-version concurrency control (MVCC)** in databases is a **different**
 * mechanism: readers often see **snapshots** without blocking writers, while
 * **`std::shared_mutex`** still **serializes** writers against readers in time.
 * Conceptually both address **read/write contention**, but MVCC trades **space**
 * (versions) and **garbage collection** for **read scalability**.
 *
 * Policy and fairness
 * -------------------
 * The standard **does not specify** whether the implementation is **reader-
 * preferred**, **writer-preferred**, or **fair**. A naive reader-heavy workload
 * can **starve writers** (or the reverse on some OS `pthread_rwlock`
 * flavors). For **latency SLAs** on writes, you may need **custom** RW locks,
 * **try-lock** with backoff, or a **different** design (copy-on-write, message
 * passing).
 *
 * When it helps / hurts
 * ---------------------
 *   • **Helps** — large read-mostly structures (routing tables, config blobs)
 *     where readers take **`shared_lock`** briefly to copy pointers or scan.
 *
 *   • **Hurts** — critical sections are **tiny**; mutex + a few atomic fields may
 *     be faster (no lock mode bookkeeping).
 *
 *   • **Hurts** — readers **hold** `shared_lock` while calling **user code** or
 *     blocking I/O → writers stall without bound.
 *
 * Upgrade / downgrade
 * --------------------
 * C++ **`std::shared_mutex` does not support** atomic “upgrade” from shared to
 * unique in one step. Patterns: **release shared**, then **acquire unique**
 * (another reader may slip in), or use **library** locks with upgrade modes, or
 * **avoid** the need by structuring data differently.
 *
 * Related patterns
 * ----------------
 *   • **Copy-on-Write** — readers never take a write lock; writers publish new
 *     snapshots (`std::atomic<std::shared_ptr<const T>>`).
 *
 *   • **RCU** — readers avoid even atomic RMW on a hot path; reclamation is
 *     deferred.
 *
 *   • **Monitor Object** — a single `mutex` protects all methods; RW lock is a
 *     refinement when access patterns split cleanly.
 *
 * Try-lock (non-blocking)
 * -----------------------
 * `std::shared_lock lk(m, std::try_to_lock)` and `std::unique_lock lk(m,
 * std::try_to_lock)` return **without blocking** if the lock is not available.
 * Use **`lk.owns_lock()`** to branch: e.g. skip work, use a fallback path, or
 * avoid deadlock in lock ordering graphs.
 *
 * Testing
 * -------
 *   • **Invariants** — under concurrent read/write, observable state should
 *     match **some** consistent write (no “torn” pairs unless you allow it).
 *
 *   • **Sanitizers** — verify no data races on protected fields.
 *
 *   • **One-definition rule** — types live in `readers_writer_lock_demo` for
 *     this `usage_examples` binary.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace usage_examples::patterns::concurrency::readers_writer_lock_demo {

	// -----------------------------------------------------------------
	// Paired fields: readers always observe a consistent (x, x) snapshot
	// -----------------------------------------------------------------
	class RwInvariantPair
	{
	public:
		[[nodiscard]] std::pair<int, int> snapshot() const
		{
			std::shared_lock lk(rw_);
			return { a_, b_ };
		}

		void publish_both(int value)
		{
			std::unique_lock lk(rw_);
			a_ = value;
			b_ = value;
		}

	private:
		mutable std::shared_mutex rw_{};
		int a_{0};
		int b_{0};
	};

	// -----------------------------------------------------------------
	// Read-mostly string config: readers copy under shared_lock
	// -----------------------------------------------------------------
	class RwStringConfig
	{
	public:
		[[nodiscard]] std::string copy_value() const
		{
			std::shared_lock lk(rw_);
			return value_;
		}

		void assign(std::string next)
		{
			std::unique_lock lk(rw_);
			value_ = std::move(next);
		}

	private:
		mutable std::shared_mutex rw_{};
		std::string value_{};
	};

	// -----------------------------------------------------------------
	// Non-blocking read: std::try_to_lock on shared_lock fails if a writer holds
	// -----------------------------------------------------------------
	class RwTryCopyBlob
	{
	public:
		[[nodiscard]] bool try_read_copy(std::string* out) const
		{
			std::shared_lock lk(rw_, std::try_to_lock);
			if (!lk.owns_lock()) return false;
			*out = body_;
			return true;
		}

		void write(std::string next)
		{
			std::unique_lock lk(rw_);
			body_ = std::move(next);
		}

		template <typename F>
		void with_exclusive(F&& fn)
		{
			std::unique_lock lk(rw_);
			std::forward<F>(fn)();
		}

	private:
		mutable std::shared_mutex rw_{};
		std::string body_{"seed"};
	};

} // namespace usage_examples::patterns::concurrency::readers_writer_lock_demo

namespace {

	using usage_examples::patterns::concurrency::readers_writer_lock_demo::RwInvariantPair;
	using usage_examples::patterns::concurrency::readers_writer_lock_demo::RwStringConfig;
	using usage_examples::patterns::concurrency::readers_writer_lock_demo::RwTryCopyBlob;

	TEST(ReadersWriterLockUsageExamples, SnapshotAlwaysShowsMatchedPair)
	{
		RwInvariantPair data;
		data.publish_both(42);
		auto p = data.snapshot();
		EXPECT_EQ(p.first, 42);
		EXPECT_EQ(p.second, 42);
	}

	TEST(ReadersWriterLockUsageExamples, ConcurrentReadersSeeInvariantUnderWriter)
	{
		RwInvariantPair data;
		std::atomic<bool> stop{ false };
		std::atomic<int> violations{ 0 };
		std::thread writer([&] {
			for (int k = 0; k < 500; ++k)
			{
				data.publish_both(k);
				std::this_thread::yield();
			}
			stop.store(true, std::memory_order_relaxed);
		});

		std::vector<std::thread> readers;
		readers.reserve(6);
		for (int r = 0; r < 6; ++r)
		{
			(void)r;
			readers.emplace_back([&] {
				while (!stop.load(std::memory_order_relaxed))
				{
					const auto [x, y] = data.snapshot();
					if (x != y) ++violations;
				}
			});
		}

		writer.join();
		for (auto& t : readers) t.join();

		EXPECT_EQ(violations.load(), 0);
	}

	TEST(ReadersWriterLockUsageExamples, StringConfigCopiesAreFullTokens)
	{
		RwStringConfig cfg;
		cfg.assign("alpha");
		EXPECT_EQ(cfg.copy_value(), "alpha");
		cfg.assign("beta");
		EXPECT_EQ(cfg.copy_value(), "beta");
	}

	TEST(ReadersWriterLockUsageExamples, ManyReadersShareLockWithOccasionalWrite)
	{
		RwStringConfig cfg;
		cfg.assign("gen0");
		std::atomic<int> reads{ 0 };

		std::vector<std::thread> pool;
		pool.reserve(8);
		for (int i = 0; i < 8; ++i)
		{
			pool.emplace_back([&cfg, &reads] {
				for (int k = 0; k < 200; ++k)
				{
					(void)cfg.copy_value();
					reads.fetch_add(1, std::memory_order_relaxed);
				}
			});
		}

		std::thread editor([&cfg] {
			for (int g = 1; g <= 10; ++g) cfg.assign("gen" + std::to_string(g));
		});

		for (auto& t : pool) t.join();
		editor.join();

		EXPECT_EQ(reads.load(), 8 * 200);
		const std::string last = cfg.copy_value();
		EXPECT_EQ(last, "gen10");
	}

	TEST(ReadersWriterLockUsageExamples, TrySharedLockFailsWhileWriterHoldsExclusive)
	{
		RwTryCopyBlob blob;
		std::atomic<bool> exclusive_active{ false };

		std::thread holder([&] {
			blob.with_exclusive([&] {
				exclusive_active.store(true, std::memory_order_relaxed);
				std::this_thread::sleep_for(std::chrono::milliseconds(30));
			});
		});

		while (!exclusive_active.load(std::memory_order_relaxed))
			std::this_thread::yield();

		std::string scratch;
		EXPECT_FALSE(blob.try_read_copy(&scratch));

		holder.join();

		EXPECT_TRUE(blob.try_read_copy(&scratch));
		EXPECT_EQ(scratch, "seed");
	}
} // namespace
