#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

namespace {

TEST(AtomicHeader, LoadStoreExchangeAndCompareExchange)
{
	// std::atomic provides lock-free or mutex-backed atomic access to T.
	std::atomic<int> a{ 1 };
	EXPECT_EQ(a.load(), 1);
	a.store(2);
	EXPECT_EQ(a.load(std::memory_order_relaxed), 2);
	EXPECT_EQ(a.exchange(3), 2);
	int expected = 3;
	EXPECT_TRUE(a.compare_exchange_strong(expected, 4));
	EXPECT_EQ(a.load(), 4);
}

TEST(AtomicHeader, AtomicFlagTestAndClear)
{
	// atomic_flag is a simple lock-free boolean with test_and_set/clear.
	std::atomic_flag f = ATOMIC_FLAG_INIT;
	EXPECT_FALSE(f.test());
	f.test_and_set();
	EXPECT_TRUE(f.test());
	f.clear();
	EXPECT_FALSE(f.test());
}

TEST(AtomicHeader, FetchAddSubAndOperators)
{
	// Integral atomics support fetch_add/fetch_sub and operator forms.
	std::atomic<int> n{ 10 };
	EXPECT_EQ(n.fetch_add(5), 10);
	EXPECT_EQ(n.load(), 15);
	n -= 3;
	EXPECT_EQ(n.load(), 12);
}

TEST(AtomicHeader, MemoryOrderFence)
{
	// std::atomic_thread_fence issues memory ordering without touching one object.
	std::atomic<int> x{ 0 };
	x.store(1, std::memory_order_relaxed);
	std::atomic_thread_fence(std::memory_order_release);
	EXPECT_EQ(x.load(std::memory_order_acquire), 1);
}

TEST(AtomicHeader, AtomicWaitNotifyWhenAvailable)
{
	// C++20 wait/notify can block until another thread changes the atomic value.
#ifdef __cpp_lib_atomic_wait
	std::atomic<int> v{ 0 };
	std::thread t([&v] {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		v.store(1, std::memory_order_release);
		v.notify_one();
	});
	v.wait(0, std::memory_order_acquire);
	EXPECT_EQ(v.load(), 1);
	t.join();
#else
	GTEST_SKIP() << "std::atomic::wait/notify not available.";
#endif
}

}  // namespace
