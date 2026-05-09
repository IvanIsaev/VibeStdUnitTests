#include <gtest/gtest.h>

#include <shared_mutex>
#include <thread>

namespace {

TEST(SharedMutexHeader, ExclusiveLockMutualExclusion)
{
	// unique_lock with shared_mutex provides exclusive writer ownership.
	std::shared_mutex mtx;
	int value = 0;

	std::thread writer([&] {
		std::unique_lock<std::shared_mutex> lock(mtx);
		value = 42;
	});

	writer.join();
	EXPECT_EQ(value, 42);
}

TEST(SharedMutexHeader, SharedLockAllowsConcurrentReaders)
{
	// shared_lock allows multiple readers if no writer holds the mutex.
	std::shared_mutex mtx;
	std::atomic<int> readers{ 0 };

	auto reader = [&] {
		std::shared_lock<std::shared_mutex> lock(mtx);
		readers.fetch_add(1, std::memory_order_relaxed);
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
		readers.fetch_sub(1, std::memory_order_relaxed);
	};

	std::thread t1(reader);
	std::thread t2(reader);
	t1.join();
	t2.join();
	EXPECT_EQ(readers.load(), 0);
}

TEST(SharedMutexHeader, TryLockSharedNonBlocking)
{
	// try_lock_shared attempts to acquire a shared lock without blocking.
	std::shared_mutex mtx;
	EXPECT_TRUE(mtx.try_lock_shared());
	EXPECT_TRUE(mtx.try_lock_shared());
	mtx.unlock_shared();
	mtx.unlock_shared();
}

}  // namespace
