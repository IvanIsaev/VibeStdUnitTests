/*
 * =============================================================================
 * Double-Checked Locking (Concurrency pattern)
 * =============================================================================
 *
 * What it is
 * ----------
 * **Double-checked locking (DCL)** is an **optimization** for **lazy
 * initialization** under concurrency:
 *
 *   1. **First check** — without holding a global mutex, test whether work is
 *      already done (e.g. pointer non-null). If yes, return fast.
 *
 *   2. **Slow path** — acquire a **mutex** so only one thread performs init.
 *
 *   3. **Second check** — under the mutex, test again: another thread may have
 *      finished initialization while this thread waited on the lock.
 *
 * Without the **second** check, every waiter would **re-run** (or duplicate)
 * initialization after the first thread left the critical section.
 *
 * Classic motivation: **singleton** or **one-time** setup of an expensive
 * resource on first use, while the **steady state** is read-heavy.
 *
 * Why the “obvious” version is wrong (pre-memory-model C++)
 * ---------------------------------------------------------
 * Textbooks once showed a pattern like:
 *
 *     if (ptr == nullptr) {           // (1) unsynchronized read
 *       std::lock_guard lk(mutex);
 *       if (ptr == nullptr)
 *         ptr = new T();               // (2) publish pointer
 *     }
 *     return ptr;
 *
 * With a **plain pointer** or **non-atomic** flag, this is **not** a valid
 * data-race-free program in C++: concurrent reads and writes to `ptr` without
 * synchronization are **undefined behavior**. Even if that were “fixed” with
 * ad-hoc platform barriers, **compiler** and **CPU** reordering can let another
 * thread observe a **non-null** `ptr` **before** the **constructor** of `T`
 * has **published** all fields — readers see a **partially constructed** object.
 *
 * C++11 and later: make it correct
 * --------------------------------
 * You need a **happens-before** edge from initialization to readers. Common
 * **correct** options:
 *
 *   • **`std::atomic<T*>`** with **`memory_order_acquire`** on the fast-path
 *     load and **`memory_order_release`** (or stronger) when storing the
 *     pointer after construction.
 *
 *   • **`std::call_once`** + **`std::once_flag`** — the standard library
 *     implements the tricky ordering; you supply the initializer.
 *
 *   • **Static local variable** in a function (**Meyers’ singleton**) — since
 *     C++11, initialization of function-local `static` is **thread-safe** and
 *     **one-shot**; the compiler emits whatever machinery is needed.
 *
 *   • **Always** take the mutex (no fast path) — correct and simple; measure
 *     before optimizing.
 *
 * Acquire / release intuition
 * ---------------------------
 *   • **`release`** on the store of the pointer: “all writes done during
 *     construction are **visible** before the pointer becomes non-null.”
 *
 *   • **`acquire`** on the load: “if I see a non-null pointer, I **synchronize
 *     with** that release and see a **fully constructed** object.”
 *
 * Weaker orders are possible in specialized designs; the acquire/release pair
 * is the usual **clear** default for publishing a lazily built object.
 *
 * When DCL still makes sense
 * ---------------------------
 *   • The **fast path** runs **very hot**; the mutex would show up in profiles.
 *
 *   • Initialization is **rare**; most calls only **read** the ready flag or
 *     pointer.
 *
 * Often **`std::call_once`** or **static locals** achieve the same correctness
 * with **less** code and **fewer** ways to get atomics wrong.
 *
 * Pitfalls
 * --------
 *   • **Destruction / lifetime** — DCL usually wires **leaky** or **process-
 *     lifetime** singletons; tearing down safely under arbitrary threads is a
 *     **different** problem (`atexit`, explicit shutdown API, etc.).
 *
 *   • **Testing** — global singletons hide **dependencies**; prefer **injecting**
 *     interfaces in new code; use DCL in examples and legacy hot paths.
 *
 *   • **Exception** in initializer — `std::call_once` **rethrows**; the flag
 *     may block retries depending on implementation; know your STL’s behavior
 *     for failed init.
 *
 * Related
 * -------
 *   • **Singleton** (creational pattern) — often **paired** with DCL in older
 *     literature; modern C++ favors **static locals** or **dependency injection**.
 *
 *   • **Readers–Writer Lock** — another pattern that combines **cheap reads**
 *     with **exclusive** work; different problem shape.
 *
 * Testing
 * -------
 * Stress **many threads** hitting the fast path: **one** initialization,
 * **identical** published pointer or value, **no** torn reads of guarded data.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

namespace usage_examples::patterns::concurrency::double_checked_locking_demo {

	// Small type built lazily (constructor side effects observable in tests)
	struct Widget
	{
		explicit Widget(int v)
			: value(v)
		{
		}
		int value;
	};

	// -----------------------------------------------------------------
	// Correct DCL: fast path uses atomic acquire; publish uses release after new
	// -----------------------------------------------------------------
	class LazyWidgetDcl
	{
	public:
		explicit LazyWidgetDcl(int ctor_arg)
			: seed_(ctor_arg)
		{
		}

		[[nodiscard]] Widget* get()
		{
			Widget* local = ptr_.load(std::memory_order_acquire);
			if (local != nullptr) return local;

			std::lock_guard<std::mutex> g(mtx_);
			local = ptr_.load(std::memory_order_relaxed);
			if (local != nullptr) return local;

			local = new Widget(seed_);
			init_count_.fetch_add(1, std::memory_order_relaxed);
			ptr_.store(local, std::memory_order_release);
			return local;
		}

		[[nodiscard]] std::size_t init_count_for_test() const
		{
			return static_cast<std::size_t>(init_count_.load(std::memory_order_relaxed));
		}

	private:
		std::mutex mtx_{};
		std::atomic<Widget*> ptr_{ nullptr };
		std::atomic<int> init_count_{ 0 };
		int seed_;
	};

	// -----------------------------------------------------------------
	// Baseline: correct but no fast path — mutex on every get()
	// -----------------------------------------------------------------
	class LazyWidgetAlwaysMutex
	{
	public:
		explicit LazyWidgetAlwaysMutex(int ctor_arg)
			: seed_(ctor_arg)
		{
		}

		[[nodiscard]] Widget* get()
		{
			std::lock_guard<std::mutex> g(mtx_);
			if (ptr_ == nullptr)
			{
				ptr_ = new Widget(seed_);
				++init_count_;
			}
			return ptr_;
		}

		[[nodiscard]] int init_count_for_test() const { return init_count_; }

	private:
		std::mutex mtx_{};
		Widget* ptr_{ nullptr };
		int init_count_{ 0 };
		int seed_;
	};

	// -----------------------------------------------------------------
	// std::call_once: library-owned correctness, minimal user atomics
	// -----------------------------------------------------------------
	class LazyCallOnceWidget
	{
	public:
		explicit LazyCallOnceWidget(int v)
			: seed_(v)
		{
		}

		[[nodiscard]] Widget* get()
		{
			std::call_once(once_, [this] {
				ptr_ = new Widget(seed_);
				++inits_;
			});
			return ptr_;
		}

		[[nodiscard]] int init_count_for_test() const { return inits_; }

	private:
		int seed_;
		std::once_flag once_{};
		Widget* ptr_{ nullptr };
		int inits_{ 0 };
	};

	// Meyers-style: function-local static (C++11 thread-safe one-time init)
	inline Widget& meyers_widget(int seed)
	{
		static Widget w(seed);
		return w;
	}

} // namespace usage_examples::patterns::concurrency::double_checked_locking_demo

namespace {

	using usage_examples::patterns::concurrency::double_checked_locking_demo::LazyWidgetAlwaysMutex;
	using usage_examples::patterns::concurrency::double_checked_locking_demo::LazyWidgetDcl;
	using usage_examples::patterns::concurrency::double_checked_locking_demo::LazyCallOnceWidget;
	using usage_examples::patterns::concurrency::double_checked_locking_demo::Widget;
	using usage_examples::patterns::concurrency::double_checked_locking_demo::meyers_widget;

	TEST(DoubleCheckedLockingUsageExamples, AtomicPointerInitializesOnceUnderContention)
	{
		LazyWidgetDcl lazy(42);
		std::vector<std::thread> threads;
		threads.reserve(16);
		std::atomic<int> sum_reads{ 0 };

		for (int i = 0; i < 16; ++i)
		{
			threads.emplace_back([&lazy, &sum_reads] {
				for (int k = 0; k < 100; ++k)
				{
					Widget* p = lazy.get();
					sum_reads.fetch_add(p->value, std::memory_order_relaxed);
				}
			});
		}

		for (auto& t : threads) t.join();

		EXPECT_EQ(lazy.init_count_for_test(), 1u);
		Widget* one = lazy.get();
		EXPECT_EQ(one->value, 42);
		EXPECT_EQ(sum_reads.load(), 16 * 100 * 42);
	}

	TEST(DoubleCheckedLockingUsageExamples, AlwaysMutexInitializesOnce)
	{
		LazyWidgetAlwaysMutex lazy(-3);
		std::vector<std::thread> threads;
		for (int i = 0; i < 8; ++i)
		{
			threads.emplace_back([&lazy] {
				for (int k = 0; k < 40; ++k) (void)lazy.get();
			});
		}
		for (auto& t : threads) t.join();
		EXPECT_EQ(lazy.init_count_for_test(), 1);
		EXPECT_EQ(lazy.get()->value, -3);
	}

	TEST(DoubleCheckedLockingUsageExamples, CallOnceInitializesOnce)
	{
		LazyCallOnceWidget gate(7);
		std::vector<std::thread> pool;
		pool.reserve(12);
		for (int t = 0; t < 12; ++t)
		{
			(void)t;
			pool.emplace_back([&gate] {
				for (int k = 0; k < 50; ++k) (void)gate.get();
			});
		}
		for (auto& th : pool) th.join();
		EXPECT_EQ(gate.init_count_for_test(), 1);
		EXPECT_EQ(gate.get()->value, 7);
	}

	TEST(DoubleCheckedLockingUsageExamples, MeyersStaticReturnsStableReference)
	{
		Widget& a = meyers_widget(100);
		Widget& b = meyers_widget(999);
		EXPECT_EQ(&a, &b);
		EXPECT_EQ(a.value, 100);
	}

} // namespace
