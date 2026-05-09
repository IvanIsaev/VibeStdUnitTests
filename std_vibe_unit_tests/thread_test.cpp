#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

namespace {

TEST(ThreadHeader, JoinRunsCallableOnNewThread)
{
	// std::thread starts concurrent execution and join() blocks for completion.
	std::atomic<int> value{ 0 };
	std::thread t([&value] { value.store(7, std::memory_order_relaxed); });
	t.join();
	EXPECT_EQ(value.load(), 7);
}

TEST(ThreadHeader, HardwareConcurrencyQuery)
{
	// hardware_concurrency returns a hint; the standard permits 0 if unknown.
	const unsigned n = std::thread::hardware_concurrency();
	(void)n;
	SUCCEED();
}

TEST(ThreadHeader, ThisThreadSleepForAndYield)
{
	// this_thread sleep/yield affect scheduling without ending the current thread.
	const auto start = std::chrono::steady_clock::now();
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	const auto elapsed = std::chrono::steady_clock::now() - start;
	EXPECT_GE(elapsed, std::chrono::milliseconds(1));

	std::this_thread::yield();
	SUCCEED();
}

TEST(ThreadHeader, JthreadJoinsOnDestructionAndStopToken)
{
	// jthread automatically joins and passes a stop_token to the thread function.
	std::atomic<bool> saw_stop{ false };
	{
		std::jthread worker([&](std::stop_token st) {
			while (!st.stop_requested())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			saw_stop.store(true, std::memory_order_relaxed);
		});
		worker.request_stop();
	}
	EXPECT_TRUE(saw_stop.load());
}

}  // namespace
