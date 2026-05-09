#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace {

TEST(ConditionVariableHeader, WaitUntilPredicate)
{
	// condition_variable waits on a unique_lock and re-checks a predicate.
	std::mutex mtx;
	std::condition_variable cv;
	bool ready = false;

	std::thread worker([&] {
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
		{
			std::lock_guard<std::mutex> lock(mtx);
			ready = true;
		}
		cv.notify_one();
	});

	{
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [&ready] { return ready; });
		EXPECT_TRUE(ready);
	}
	worker.join();
}

TEST(ConditionVariableHeader, WaitForWithTimeout)
{
	// wait_for returns if the timeout elapses without notification.
	std::mutex mtx;
	std::condition_variable cv;
	std::unique_lock<std::mutex> lock(mtx);
	const auto status = cv.wait_for(lock, std::chrono::milliseconds(1), [] { return false; });
	EXPECT_EQ(status, std::cv_status::timeout);
}

TEST(ConditionVariableHeader, NotifyAllWakesMultipleWaiters)
{
	// notify_all unblocks all threads waiting on the same condition_variable.
	std::mutex mtx;
	std::condition_variable cv;
	int count = 0;
	constexpr int kWaiters = 3;

	auto waiter = [&] {
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [&count] { return count > 0; });
	};

	std::thread t1(waiter);
	std::thread t2(waiter);
	std::thread t3(waiter);

	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	{
		std::lock_guard<std::mutex> lock(mtx);
		count = 1;
	}
	cv.notify_all();

	t1.join();
	t2.join();
	t3.join();
}

}  // namespace
