#include <gtest/gtest.h>

#include <mutex>
#include <thread>

namespace {

TEST(MutexHeader, MutexLockUnlockSerializesCriticalSection)
{
	// std::mutex provides exclusive non-recursive ownership.
	std::mutex mtx;
	int counter = 0;

	std::thread t([&] {
		std::lock_guard<std::mutex> lock(mtx);
		counter += 10;
	});

	{
		std::lock_guard<std::mutex> lock(mtx);
		counter += 1;
	}
	t.join();
	EXPECT_EQ(counter, 11);
}

TEST(MutexHeader, UniqueLockDeferredLockingAndAdopt)
{
	// unique_lock supports deferred locking and adopting already-held mutexes.
	std::mutex mtx;
	std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
	EXPECT_FALSE(lock.owns_lock());
	lock.lock();
	EXPECT_TRUE(lock.owns_lock());
	lock.unlock();

	mtx.lock();
	std::unique_lock<std::mutex> adopted(mtx, std::adopt_lock);
	EXPECT_TRUE(adopted.owns_lock());
}

TEST(MutexHeader, RecursiveMutexAllowsReentrantLocking)
{
	// std::recursive_mutex permits the same thread to lock multiple levels deep.
	std::recursive_mutex mtx;
	std::lock_guard<std::recursive_mutex> outer(mtx);
	std::lock_guard<std::recursive_mutex> inner(mtx);
	SUCCEED();
}

TEST(MutexHeader, TimedMutexTryLockFor)
{
	// timed_mutex supports try_lock_for with chrono timeouts.
	std::timed_mutex mtx;
	EXPECT_TRUE(mtx.try_lock());
	EXPECT_FALSE(mtx.try_lock_for(std::chrono::milliseconds(1)));
	mtx.unlock();
}

TEST(MutexHeader, ScopedLockLocksMultipleMutexesDeadlockFree)
{
	// std::scoped_lock acquires one or more mutexes in a safe total order.
	std::mutex a;
	std::mutex b;
	{
		std::scoped_lock lock(a, b);
		SUCCEED();
	}
}

TEST(MutexHeader, TryLockVariadicHelper)
{
	// std::try_lock returns the index of the first mutex it failed to lock, or -1.
	std::mutex m0;
	std::mutex m1;
	const int r = std::try_lock(m0, m1);
	EXPECT_EQ(r, -1);
	m0.unlock();
	m1.unlock();
}

TEST(MutexHeader, CallOnceRunsInitializerExactlyOnce)
{
	// std::once_flag + std::call_once guarantee single initialization across threads.
	std::once_flag flag;
	int counter = 0;
	auto init = [&counter] { ++counter; };
	std::call_once(flag, init);
	std::call_once(flag, init);
	EXPECT_EQ(counter, 1);
}

}  // namespace
