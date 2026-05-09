#include <gtest/gtest.h>

#include <chrono>
#include <semaphore>
#include <thread>

namespace {

TEST(SemaphoreHeader, BinarySemaphoreAcquireRelease)
{
	// binary_semaphore models a single-slot counting semaphore (max 1).
	std::binary_semaphore gate{ 0 };
	std::thread releaser([&gate] {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		gate.release();
	});
	gate.acquire();
	releaser.join();
}

TEST(SemaphoreHeader, CountingSemaphoreTryAcquireFor)
{
	// counting_semaphore generalizes to N simultaneous permits.
	std::counting_semaphore<> sem(2);
	EXPECT_TRUE(sem.try_acquire());
	EXPECT_TRUE(sem.try_acquire());
	EXPECT_FALSE(sem.try_acquire_for(std::chrono::milliseconds(1)));
	sem.release(2);
}

TEST(SemaphoreHeader, ReleaseIncrementsAvailablePermits)
{
	// release(n) adds n permits; acquire consumes one permit.
	std::counting_semaphore<> sem(0);
	sem.release(3);
	sem.acquire();
	sem.acquire();
	sem.acquire();
}

}  // namespace
