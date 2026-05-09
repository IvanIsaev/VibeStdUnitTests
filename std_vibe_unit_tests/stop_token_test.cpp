#include <gtest/gtest.h>

#include <atomic>
#include <stop_token>
#include <thread>

namespace {

TEST(StopTokenHeader, StopSourceRequestsStop)
{
	// stop_source/stop_token coordinate cooperative cancellation.
	std::stop_source src;
	std::stop_token tok = src.get_token();
	EXPECT_FALSE(tok.stop_requested());
	src.request_stop();
	EXPECT_TRUE(tok.stop_requested());
}

TEST(StopTokenHeader, StopCallbackInvokedOnRequestStop)
{
	// stop_callback registers a callable that runs when cancellation is requested.
	std::stop_source src;
	std::atomic<bool> invoked{ false };
	{
		std::stop_callback cb(src.get_token(), [&invoked] { invoked.store(true, std::memory_order_relaxed); });
		EXPECT_FALSE(invoked.load());
		src.request_stop();
	}
	EXPECT_TRUE(invoked.load());
}

TEST(StopTokenHeader, TokenCanBeDefaultConstructed)
{
	// A default-constructed stop_token is never associated with a stop state.
	std::stop_token t;
	EXPECT_FALSE(t.stop_possible());
	EXPECT_FALSE(t.stop_requested());
}

}  // namespace
