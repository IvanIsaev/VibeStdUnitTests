#include <gtest/gtest.h>

#include <latch>
#include <thread>
#include <vector>

namespace {

TEST(LatchHeader, CountDownReleasesWaiters)
{
	// std::latch is a single-use counter; wait() blocks until count reaches zero.
	constexpr int kThreads = 4;
	std::latch done(kThreads);
	std::atomic<int> started{ 0 };

	std::vector<std::thread> threads;
	for (int i = 0; i < kThreads; ++i)
	{
		threads.emplace_back([&done, &started] {
			started.fetch_add(1, std::memory_order_relaxed);
			done.count_down();
			done.wait();
		});
	}

	for (auto& t : threads)
	{
		t.join();
	}
	EXPECT_EQ(started.load(), kThreads);
}

TEST(LatchHeader, TryWaitObservesArrivalWithoutBlockingForever)
{
	// try_wait returns immediately whether the latch has opened.
	std::latch gate(1);
	EXPECT_FALSE(gate.try_wait());
	gate.count_down();
	EXPECT_TRUE(gate.try_wait());
}

}  // namespace
