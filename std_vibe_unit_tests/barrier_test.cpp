#include <gtest/gtest.h>

#include <barrier>
#include <thread>
#include <vector>

namespace {

TEST(BarrierHeader, ArriveAndWaitSynchronizesThreads)
{
	// std::barrier blocks a team until all threads arrive, then runs completion.
	constexpr int kThreads = 3;
	std::barrier sync(kThreads);
	std::atomic<int> phase{ 0 };

	std::vector<std::thread> threads;
	for (int i = 0; i < kThreads; ++i)
	{
		threads.emplace_back([&sync, &phase] {
			phase.fetch_add(1, std::memory_order_relaxed);
			sync.arrive_and_wait();
			phase.fetch_add(10, std::memory_order_relaxed);
		});
	}
	for (auto& t : threads)
	{
		t.join();
	}
	EXPECT_EQ(phase.load(), kThreads + kThreads * 10);
}

TEST(BarrierHeader, CompletionFunctionRunsOncePerPhase)
{
	// The completion callable runs on one arriving thread after the phase completes.
	std::atomic<int> completions{ 0 };
	auto on_complete = [&completions]() noexcept { completions.fetch_add(1, std::memory_order_relaxed); };
	std::barrier<decltype(on_complete)> sync(2, on_complete);

	std::thread a([&sync] { sync.arrive_and_drop(); });
	std::thread b([&sync] { sync.arrive_and_drop(); });
	a.join();
	b.join();
	EXPECT_EQ(completions.load(), 1);
}

}  // namespace
