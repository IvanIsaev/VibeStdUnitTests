/*
 * =============================================================================
 * Object Pool (performance / creational idiom; grouped here with usage examples)
 * =============================================================================
 *
 * Intent
 * ------
 * Keep a **reservoir of pre-constructed objects** (or memory blocks) that can be
 * **lent** to clients and **returned** when work finishes, instead of paying
 * allocation + construction cost on every use. The pool owns the storage; clients
 * borrow handles for a scope or until they explicitly release.
 *
 * This pattern is **not** one of the original Gang of Four names, but it appears
 * everywhere in production: graphics buffers, particle systems, database
 * connection pools, socket/session reuse, and game entity recycling.
 *
 * Core ideas
 * ----------
 *   • **Acquire** — take an idle object from the pool (or fail / wait / grow).
 *   • **Release** — return the object to the pool after resetting invariants.
 *   • **Warm-up** — optionally pre-create objects at startup to avoid spikes.
 *
 * When Object Pool helps
 * ----------------------
 *   • Object **construction or teardown is expensive** (kernel handles, GPU
 *     resources, parsers with heavy buffers).
 *   • Allocation churn causes **fragmentation** or **GC** pressure in managed
 *     runtimes (here: heap churn in hot loops).
 *   • You need **bounded resource usage** — cap concurrent live objects.
 *
 * When to skip it
 * ---------------
 *   • Cheap default-constructible types — a local `std::vector` or `string` may
 *     be simpler and fast enough with a good allocator.
 *   • **Hard-to-reset** objects — if clearing state is error-prone, fresh
 *     instances can be safer than pooling.
 *   • Very **uneven** sizes — one pool per size class, or use arenas instead.
 *
 * Object Pool vs Flyweight
 * ------------------------
 *   • **Flyweight** shares *immutable* extrinsic state across many logical
 *     objects (many "characters" referencing one glyph bitmap).
 *   • **Object Pool** reuses *mutable* instances over time; only one borrower
 *     should own a pooled object at a time (unless you add sharing explicitly).
 *
 * Object Pool vs caching
 * ----------------------
 *   • A **cache** maps keys → values and may evict by policy (LRU).
 *   • A **pool** typically has **no key**; any idle instance is interchangeable
 *     after `prepare_for_reuse()`.
 *
 * Design choices in C++
 * ---------------------
 *   • **API shape:** raw `acquire` / `release` vs `std::unique_ptr` with a
 *     **custom deleter** that returns to the pool (RAII-friendly).
 *
 *   • **Exhaustion:** return `nullptr` / `std::optional`, block on a condition
 *     variable, or allocate a fresh object up to a **hard cap**.
 *
 *   • **Thread safety:** one mutex for the free-list is simple; **lock-free
 *     stacks** help when profiling shows contention (not shown here).
 *
 *   • **Lifetime:** the pool must **outlive** borrowed objects unless you
 *     document transfer of ownership on shutdown (usually drain the pool
 *     after all handles are returned).
 *
 * Resetting pooled objects
 * ------------------------
 * Always define **`prepare_for_reuse()`** (or equivalent) so no stale data
 * leaks to the next borrower. Call it **on release**, not on acquire, so
 * borrowers never see half-cleared state if they still hold a pointer (if you
 * only support RAII handles, acquire-time reset is also fine — pick one policy
 * and document it).
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   // Custom deleter returns a buffer to its pool:
 *   auto deleter = [pool](BigBuffer* p) { pool->release(p); };
 *   std::unique_ptr<BigBuffer, decltype(deleter)> buf(pool->acquire_raw(), deleter);
 *
 *   // Fixed-size pool of DB connections (pseudocode):
 *   class ConnectionPool {
 *    public:
 *     std::unique_ptr<Connection> checkout();  // blocks or times out
 *     void checkin(std::unique_ptr<Connection>);
 *    private:
 *     std::queue<std::unique_ptr<Connection>> idle_;
 *     std::mutex m_;
 *   };
 *
 *   // Free-list of indices into a vector (no pointer chasing):
 *   std::vector<Entity> storage_;
 *   std::vector<int> free_indices_;
 *   int acquire_index() { int i = free_indices_.back(); free_indices_.pop_back(); return i; }
 *
 * Pitfalls
 * --------
 *   • **Dropping `std::unique_ptr` without `release()`** — with
 *     `std::default_delete`, the object is destroyed and leaves the pool's
 *     capacity out of sync; use a **custom deleter** or `ScopedPooledBuffer`.
 *   • **Double release** or releasing an object not from this pool — use RAII
 *     handles and private constructors if needed.
 *   • **Oversized pools** — idle objects still consume memory; tune prealloc.
 *   • **Forgotten `prepare_for_reuse`** — security and logic bugs from stale data.
 *   • **Pool destroyed while borrows live** — UB unless you enforce shutdown order.
 *
 * Testing
 * -------
 *   • Exhaust the pool, verify acquire fails, then return one object and verify
 *     acquire succeeds.
 *   • After release, assert reused buffer has cleared fields if your policy
 *     clears on release.
 *   • Stress with concurrent acquire/release if your pool is shared.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace usage_examples::patterns::behavioral {

	// -----------------------------------------------------------------
	// Pooled type: must be cheap to "clear" for the next user.
	// -----------------------------------------------------------------
	struct MessageBuffer
	{
		std::string payload;
		int priority = 0;

		void prepare_for_reuse()
		{
			payload.clear();
			priority = 0;
		}
	};

	// -----------------------------------------------------------------
	// Fixed-capacity pool: at most `capacity` objects exist; acquire fails when
	// all are checked out and none are idle.
	// -----------------------------------------------------------------
	class MessageBufferPool
	{
	public:
		explicit MessageBufferPool(std::size_t capacity) : capacity_(capacity)
		{
			if (capacity == 0) throw std::invalid_argument("pool capacity must be > 0");
			for (std::size_t i = 0; i < capacity_; ++i)
				idle_.push_back(std::make_unique<MessageBuffer>());
		}

		MessageBufferPool(const MessageBufferPool&) = delete;
		MessageBufferPool& operator=(const MessageBufferPool&) = delete;

		[[nodiscard]] std::size_t capacity() const { return capacity_; }

		[[nodiscard]] std::size_t available_approx() const
		{
			std::lock_guard<std::mutex> lock(mutex_);
			return idle_.size();
		}

		[[nodiscard]] std::size_t checked_out_approx() const
		{
			std::lock_guard<std::mutex> lock(mutex_);
			return capacity_ - idle_.size();
		}

		// Removes one idle buffer, or returns nullptr if the pool is exhausted.
		[[nodiscard]] std::unique_ptr<MessageBuffer> try_acquire()
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (idle_.empty()) return nullptr;
			auto node = std::move(idle_.back());
			idle_.pop_back();
			return node;
		}

		void release(std::unique_ptr<MessageBuffer> buffer)
		{
			if (!buffer) return;
			buffer->prepare_for_reuse();
			std::lock_guard<std::mutex> lock(mutex_);
			if (idle_.size() >= capacity_) throw std::logic_error("release exceeds pool capacity — foreign pointer?");
			idle_.push_back(std::move(buffer));
		}

	private:
		const std::size_t capacity_;
		mutable std::mutex mutex_;
		std::vector<std::unique_ptr<MessageBuffer>> idle_;
	};

	// -----------------------------------------------------------------
	// RAII handle: always returns the buffer to the pool on destruction.
	// -----------------------------------------------------------------
	class ScopedPooledBuffer
	{
	public:
		ScopedPooledBuffer(MessageBufferPool& pool, std::unique_ptr<MessageBuffer> buffer)
			: pool_(&pool), buffer_(std::move(buffer))
		{}

		ScopedPooledBuffer(const ScopedPooledBuffer&) = delete;
		ScopedPooledBuffer& operator=(const ScopedPooledBuffer&) = delete;

		ScopedPooledBuffer(ScopedPooledBuffer&& other) noexcept
			: pool_(other.pool_), buffer_(std::move(other.buffer_))
		{
			other.pool_ = nullptr;
		}

		ScopedPooledBuffer& operator=(ScopedPooledBuffer&& other) noexcept
		{
			if (this != &other)
			{
				reset();
				pool_ = other.pool_;
				buffer_ = std::move(other.buffer_);
				other.pool_ = nullptr;
			}
			return *this;
		}

		~ScopedPooledBuffer() { reset(); }

		[[nodiscard]] MessageBuffer* get() const { return buffer_.get(); }
		[[nodiscard]] MessageBuffer* operator->() const { return buffer_.get(); }

		void reset()
		{
			if (buffer_ && pool_) pool_->release(std::move(buffer_));
			pool_ = nullptr;
		}

	private:
		MessageBufferPool* pool_{};
		std::unique_ptr<MessageBuffer> buffer_;
	};

	// -----------------------------------------------------------------
	// Growing pool (capped): creates new objects when idle is empty until
	// `live` reaches max_live; release returns objects to the idle stack.
	// -----------------------------------------------------------------
	class GrowingBufferPool
	{
	public:
		explicit GrowingBufferPool(std::size_t max_live) : max_live_(max_live)
		{
			if (max_live == 0) throw std::invalid_argument("max_live must be > 0");
		}

		[[nodiscard]] std::unique_ptr<MessageBuffer> try_acquire()
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (!idle_.empty())
			{
				auto b = std::move(idle_.back());
				idle_.pop_back();
				return b;
			}
			if (live_ >= max_live_) return nullptr;
			++live_;
			return std::make_unique<MessageBuffer>();
		}

		void release(std::unique_ptr<MessageBuffer> buffer)
		{
			if (!buffer) return;
			buffer->prepare_for_reuse();
			std::lock_guard<std::mutex> lock(mutex_);
			idle_.push_back(std::move(buffer));
		}

		[[nodiscard]] std::size_t live_count_approx() const
		{
			std::lock_guard<std::mutex> lock(mutex_);
			return live_;
		}

	private:
		const std::size_t max_live_;
		mutable std::mutex mutex_;
		std::size_t live_{ 0 };
		std::vector<std::unique_ptr<MessageBuffer>> idle_;
	};

} // namespace usage_examples::patterns::behavioral

namespace {

	using usage_examples::patterns::behavioral::GrowingBufferPool;
	using usage_examples::patterns::behavioral::MessageBufferPool;
	using usage_examples::patterns::behavioral::ScopedPooledBuffer;

	TEST(ObjectPoolUsageExamples, ExhaustThenReleaseAllowsReuse)
	{
		MessageBufferPool pool(2);
		EXPECT_EQ(pool.available_approx(), 2u);

		auto a = pool.try_acquire();
		auto b = pool.try_acquire();
		ASSERT_TRUE(a && b);
		EXPECT_EQ(pool.try_acquire(), nullptr);

		a->payload = "first";
		b->priority = 7;

		pool.release(std::move(a));
		auto c = pool.try_acquire();
		ASSERT_NE(c, nullptr);
		pool.release(std::move(c));

		pool.release(std::move(b));
	}

	TEST(ObjectPoolUsageExamples, ReleaseClearsPayloadForNextBorrower)
	{
		MessageBufferPool pool(1);
		auto raw = pool.try_acquire();
		ASSERT_TRUE(raw);
		raw->payload = "secret";
		raw->priority = 99;
		pool.release(std::move(raw));

		auto again = pool.try_acquire();
		ASSERT_TRUE(again);
		EXPECT_TRUE(again->payload.empty());
		EXPECT_EQ(again->priority, 0);
	}

	TEST(ObjectPoolUsageExamples, ScopedHandleReturnsToPoolAutomatically)
	{
		MessageBufferPool pool(1);
		{
			ScopedPooledBuffer scoped(pool, pool.try_acquire());
			ASSERT_TRUE(scoped.get());
			scoped->payload = "tmp";
		}
		auto reused = pool.try_acquire();
		ASSERT_TRUE(reused);
		EXPECT_TRUE(reused->payload.empty());
	}

	TEST(ObjectPoolUsageExamples, GrowingPoolAllocatesUntilCap)
	{
		GrowingBufferPool pool(2);
		auto x = pool.try_acquire();
		auto y = pool.try_acquire();
		ASSERT_TRUE(x && y);
		EXPECT_EQ(pool.try_acquire(), nullptr);

		pool.release(std::move(x));
		auto z = pool.try_acquire();
		ASSERT_TRUE(z);
	}

	TEST(ObjectPoolUsageExamples, ConcurrentAcquireReleaseIsStressSmokeTest)
	{
		MessageBufferPool pool(4);
		std::atomic<int> ok{ 0 };
		auto worker = [&] {
			for (int i = 0; i < 50; ++i)
			{
				auto p = pool.try_acquire();
				if (!p) continue;
				p->payload.push_back('x');
				pool.release(std::move(p));
				ok.fetch_add(1, std::memory_order_relaxed);
			}
		};
		std::jthread t1(worker);
		std::jthread t2(worker);
		t1.join();
		t2.join();
		EXPECT_GT(ok.load(), 0);
		EXPECT_EQ(pool.available_approx(), 4u);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Replace `std::mutex` with a lock-free stack for `try_acquire`/`release` if
 *    profiling shows lock contention on a hot path.
 * 2. Add `std::condition_variable` and `acquire_blocking()` with timeout for
 *    connection-pool semantics.
 * 3. Integrate with an **arena**: pooled objects are slices from a few large
 *    blocks to improve locality (common in games and networking stacks).
 */
