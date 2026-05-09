#include <gtest/gtest.h>

#include <future>
#include <numeric>
#include <vector>

namespace {

TEST(FutureHeader, PromiseAndFutureTransferResult)
{
	// promise/future pairs communicate a single deferred value across threads.
	std::promise<int> p;
	std::future<int> f = p.get_future();
	p.set_value(42);
	EXPECT_EQ(f.get(), 42);
}

TEST(FutureHeader, SharedFutureMultipleReaders)
{
	// shared_future allows multiple threads to read the same shared state.
	std::promise<int> p;
	std::shared_future<int> sf = p.get_future().share();
	p.set_value(7);
	EXPECT_EQ(sf.get(), 7);
	EXPECT_EQ(sf.get(), 7);
}

TEST(FutureHeader, PackagedTaskWrapsCallable)
{
	// packaged_task stores a callable and produces a future for its result.
	std::packaged_task<int(int, int)> task([](int a, int b) { return a + b; });
	std::future<int> fut = task.get_future();
	task(3, 4);
	EXPECT_EQ(fut.get(), 7);
}

TEST(FutureHeader, AsyncRunsFunctionAsynchronously)
{
	// std::async may launch a new thread and return a future for the result.
	std::vector<int> data(1000);
	std::iota(data.begin(), data.end(), 0);
	std::future<int> sum = std::async(std::launch::async, [&data] {
		return std::accumulate(data.begin(), data.end(), 0);
	});
	EXPECT_EQ(sum.get(), 499500);
}

TEST(FutureHeader, AsyncDeferredPolicy)
{
	// launch::deferred evaluates the callable when the future is waited on.
	std::atomic<bool> ran{ false };
	std::future<void> f = std::async(std::launch::deferred, [&ran] { ran = true; });
	EXPECT_FALSE(ran.load());
	f.wait();
	EXPECT_TRUE(ran.load());
}

TEST(FutureHeader, FutureWaitForReturnsReadyStatus)
{
	// wait_for polls completion with a timeout and reports readiness via future_status.
	std::promise<void> p;
	std::future<void> f = p.get_future();
	EXPECT_EQ(f.wait_for(std::chrono::milliseconds(1)), std::future_status::timeout);
	p.set_value();
	EXPECT_EQ(f.wait_for(std::chrono::milliseconds(1)), std::future_status::ready);
}

}  // namespace
