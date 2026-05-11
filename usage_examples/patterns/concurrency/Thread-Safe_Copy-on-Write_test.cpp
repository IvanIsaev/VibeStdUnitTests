/*
 * =============================================================================
 * Thread-Safe Copy-on-Write (COW)
 * =============================================================================
 *
 * What it is
 * ----------
 * **Copy-on-Write** lets **many readers** share one underlying **immutable
 * snapshot** cheaply (pointer copy + atomic refcount). A **writer** only pays
 * for a full duplicate when it mutates—either because the buffer is **shared**
 * (`use_count() > 1` in classic single-threaded COW) or because you **always
 * clone** on write for simpler thread safety.
 *
 * In concurrent programs the goal is:
 *
 *   • **Readers** never block each other (ideally **lock-free** reads).
 *
 *   • **Writers** publish a **new** snapshot so existing reader handles keep
 *     seeing a **stable** view until they refresh.
 *
 *   • **No torn reads** of large structures—clients iterate a `shared_ptr` to
 *     `const` data that stays valid for the lifetime of their local handle.
 *
 * Why `shared_ptr<const T>`?
 * --------------------------
 * Holding `std::shared_ptr<const std::vector<int>>` (or `const` aggregates)
 * signals **read-only sharing**. Writers allocate a **new** modifiable buffer,
 * then **atomically** publish it. Old snapshots remain valid for in-flight
 * readers until they drop their handles.
 *
 * Lock-free publication (C++20)
 * ------------------------------
 * `std::atomic<std::shared_ptr<const T>>` provides **atomic load/store/CAS** on
 * the *smart pointer object* (not on individual vector elements). Combined with
 * **compare-exchange retry loops**, writers can race safely: if another thread
 * publishes first, reload and base your edit on the latest snapshot.
 *
 * This is **not** a magic “atomic vector”: element-wise updates still require
 * your usual mutex, transactions, or data structures designed for parallelism.
 *
 * Mutex-based COW (simpler mental model)
 * ----------------------------------------
 * A single **mutex** guards replacement of the `shared_ptr`. Readers take the
 * mutex briefly to **copy** the pointer (or use `shared_lock` with a
 * `shared_mutex` if you split read/write locking). Writers clone under an
 * **exclusive** lock and swap. Fewer subtle CAS details; usually **more
 * contention** on the hot path.
 *
 * Classic single-threaded COW (for contrast)
 * ------------------------------------------
 * `std::vector` + `std::shared_ptr` + `unique()` to mutate **in place** when
 * the last owner writes. **Do not** use `unique()` as a thread-safety test—
 * another thread may concurrently copy the `shared_ptr` and bump the count.
 *
 * Pitfalls
 * --------
 *   • **Write amplification** — every mutation clones the whole buffer; huge
 *     snapshots make COW expensive. Consider **chunking**, **persistent
 *     structures**, or **double buffering**.
 *
 *   • **ABA / lost updates** — without CAS retry (or a lock), interleaved
 *     writers can drop edits. The loop in `atomic_compare_exchange_strong`
 *     fixes this for pointer replacement.
 *
 *   • **Iteration + write** — iterating a snapshot while writers publish newer
 *     versions is fine; you simply see **older** state until you reload.
 *
 *   • **Destructors and callbacks** — delaying reclamation until the last
 *     `shared_ptr` drops can surprise you if teardown is costly; consider
 *     `std::make_shared` + custom allocators or hazard pointers for exotic cases.
 *
 * Related ideas
 * -------------
 *   • **RCU** (read-copy-update) — kernel/user-space RCU delays freeing old
 *     versions until quiescent points.
 *
 *   • **Immutable persistent collections** — structural sharing reduces clone cost.
 *
 *   • **Double buffering / swap chains** — graphics and audio engines publish
 *     full frames atomically.
 *
 * Testing
 * -------
 *   • **Snapshot stability** — after `auto old = cow.snapshot(); cow.mutate();`
 *     assert `old` still reflects the pre-mutation value.
 *
 *   • **Concurrent writers** — stress with many threads; final state should match
 *     serialized replay (e.g., multiset of operations).
 *
 *   • **Sanitizers** — run ThreadSanitizer / MSVC `/fsanitize=thread` on stress
 *     tests when available.
 *
 *   • **One-definition rule** — types live in `ts_cow_demo` inside this TU’s
 *     namespace to avoid clashing with other `usage_examples` sources.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

namespace usage_examples::patterns::concurrency::ts_cow_demo {

	// -----------------------------------------------------------------
	// Lock-free style publication (C++20 atomic<shared_ptr<const T>>)
	// -----------------------------------------------------------------
	class AtomicCowIntVector
	{
	public:
		AtomicCowIntVector()
		{
			slot_.store(std::make_shared<const std::vector<int>>(), std::memory_order_relaxed);
		}

		[[nodiscard]] std::shared_ptr<const std::vector<int>> snapshot() const
		{
			return slot_.load(std::memory_order_acquire);
		}

		void push_back(int value)
		{
			auto cur = slot_.load(std::memory_order_acquire);
			for (;;)
			{
				auto edited = std::make_shared<std::vector<int>>(*cur);
				edited->push_back(value);
				std::shared_ptr<const std::vector<int>> desired(edited);
				if (slot_.compare_exchange_strong(cur, desired, std::memory_order_acq_rel, std::memory_order_acquire))
					return;
			}
		}

		void append_range(std::vector<int> chunk)
		{
			auto cur = slot_.load(std::memory_order_acquire);
			for (;;)
			{
				auto edited = std::make_shared<std::vector<int>>(*cur);
				edited->insert(edited->end(), chunk.begin(), chunk.end());
				std::shared_ptr<const std::vector<int>> desired(edited);
				if (slot_.compare_exchange_strong(cur, desired, std::memory_order_acq_rel, std::memory_order_acquire))
					return;
			}
		}

	private:
		mutable std::atomic<std::shared_ptr<const std::vector<int>>> slot_{};
	};

	// -----------------------------------------------------------------
	// Mutex + shared_ptr<const string> — easy to reason about; readers share lock
	// -----------------------------------------------------------------
	class SharedMutexCowString
	{
	public:
		SharedMutexCowString() : data_(std::make_shared<const std::string>()) {}

		[[nodiscard]] std::shared_ptr<const std::string> snapshot() const
		{
			std::shared_lock lk(rw_);
			return data_;
		}

		void append(std::string_view suffix)
		{
			std::unique_lock lk(rw_);
			auto base = std::make_shared<std::string>(*data_);
			base->append(suffix);
			data_ = std::move(base);
		}

	private:
		mutable std::shared_mutex rw_{};
		std::shared_ptr<const std::string> data_{};
	};

	// -----------------------------------------------------------------
	// Tiny aggregate published atomically — “replace whole config” pattern
	// -----------------------------------------------------------------
	struct DisplayConfig
	{
		int brightness{};
		int contrast{};
		bool dark_mode{};
	};

	class AtomicCowDisplayConfig
	{
	public:
		AtomicCowDisplayConfig()
		{
			slot_.store(std::make_shared<const DisplayConfig>(), std::memory_order_relaxed);
		}

		[[nodiscard]] std::shared_ptr<const DisplayConfig> snapshot() const
		{
			return slot_.load(std::memory_order_acquire);
		}

		void publish(DisplayConfig next)
		{
			auto replacement = std::make_shared<const DisplayConfig>(next);
			slot_.store(replacement, std::memory_order_release);
		}

	private:
		mutable std::atomic<std::shared_ptr<const DisplayConfig>> slot_{};
	};

} // namespace usage_examples::patterns::concurrency::ts_cow_demo

namespace {

	using usage_examples::patterns::concurrency::ts_cow_demo::AtomicCowDisplayConfig;
	using usage_examples::patterns::concurrency::ts_cow_demo::AtomicCowIntVector;
	using usage_examples::patterns::concurrency::ts_cow_demo::DisplayConfig;
	using usage_examples::patterns::concurrency::ts_cow_demo::SharedMutexCowString;

	TEST(ThreadSafeCopyOnWriteUsageExamples, SnapshotRemainsStableAfterMutation)
	{
		AtomicCowIntVector cow;
		auto frozen = cow.snapshot();
		ASSERT_TRUE(frozen);
		EXPECT_TRUE(frozen->empty());

		cow.push_back(7);
		cow.push_back(8);

		ASSERT_TRUE(frozen);
		EXPECT_TRUE(frozen->empty());

		auto latest = cow.snapshot();
		ASSERT_TRUE(latest);
		ASSERT_EQ(latest->size(), 2u);
		EXPECT_EQ((*latest)[0], 7);
		EXPECT_EQ((*latest)[1], 8);
	}

	TEST(ThreadSafeCopyOnWriteUsageExamples, ConcurrentPushBackPreservesAllValues)
	{
		AtomicCowIntVector cow;
		constexpr int kThreads = 8;
		constexpr int kPushesPerThread = 250;
		std::vector<std::thread> workers;
		workers.reserve(kThreads);

		for (int t = 0; t < kThreads; ++t)
		{
			workers.emplace_back([&, t] {
				for (int i = 0; i < kPushesPerThread; ++i) cow.push_back(t * kPushesPerThread + i);
			});
		}

		for (auto& w : workers) w.join();

		auto final_snap = cow.snapshot();
		ASSERT_TRUE(final_snap);
		EXPECT_EQ(final_snap->size(), static_cast<unsigned>(kThreads * kPushesPerThread));

		std::vector<int> sorted(final_snap->begin(), final_snap->end());
		std::sort(sorted.begin(), sorted.end());
		for (int i = 0; i < static_cast<int>(sorted.size()); ++i) EXPECT_EQ(sorted[i], i);
	}

	TEST(ThreadSafeCopyOnWriteUsageExamples, SharedMutexStringCowAppends)
	{
		SharedMutexCowString log;
		auto a = log.snapshot();
		EXPECT_TRUE(a->empty());

		log.append("hello");
		log.append(", ");
		log.append("cow");

		EXPECT_TRUE(a->empty());

		auto b = log.snapshot();
		ASSERT_TRUE(b);
		EXPECT_EQ(*b, "hello, cow");
	}

	TEST(ThreadSafeCopyOnWriteUsageExamples, AtomicReplacePublishesWholeStruct)
	{
		AtomicCowDisplayConfig cfg;
		auto old = cfg.snapshot();
		EXPECT_EQ(old->brightness, 0);

		cfg.publish(DisplayConfig{ 80, 55, true });
		EXPECT_EQ(old->brightness, 0);

		auto neu = cfg.snapshot();
		EXPECT_EQ(neu->brightness, 80);
		EXPECT_EQ(neu->contrast, 55);
		EXPECT_TRUE(neu->dark_mode);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add a **TSAN**-gated stress test that hammers `push_back` and `snapshot` in
 *    parallel readers and assert reference stability (no iterator crashes).
 * 2. Implement **amortized COW** for strings: rope or chunk list so append is
 *    O(1) amortized instead of cloning the entire buffer.
 * 3. Compare with `std::shared_mutex` guarding a plain `std::vector` (no COW) —
 *    measure reader latency under write bursts.
 * 4. Pair with **version counters** so readers can detect stale snapshots cheaply
 *    (`if (version != global) reload`).
 */
