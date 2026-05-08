#include <gtest/gtest.h>

#include <csignal>

namespace {

volatile std::sig_atomic_t g_handlerCallCount = 0;
volatile std::sig_atomic_t g_lastSignal = 0;

void CountingSignalHandler(int signalValue)
{
	// Signal handlers are restricted contexts where only async-signal-safe
	// operations are valid. Writing to volatile sig_atomic_t is the canonical
	// portable operation for communicating back to normal code.
	++g_handlerCallCount;
	g_lastSignal = static_cast<std::sig_atomic_t>(signalValue);
}

TEST(CSignal, SigAtomicTSupportsAtomicLikeSignalCommunication)
{
	// sig_atomic_t is the dedicated type for values shared with a signal handler.
	// It guarantees that reads/writes are not torn with respect to asynchronous
	// signal interruption, so simple state handoff through this type is safe.
	volatile std::sig_atomic_t value = 0;
	value = 123;
	EXPECT_EQ(value, 123);
}

TEST(CSignal, SignalAndRaiseDeliverHandledSignal)
{
	// signal installs a handler for a signal number, and raise sends that signal
	// to the current process. This test verifies the core round-trip:
	//   1) install custom SIGINT handler,
	//   2) raise SIGINT,
	//   3) observe handler side effects,
	//   4) restore previous disposition to avoid cross-test interference.
	g_handlerCallCount = 0;
	g_lastSignal = 0;

	void (*previousHandler)(int) = std::signal(SIGINT, CountingSignalHandler);
	ASSERT_NE(previousHandler, SIG_ERR);

	const int raiseResult = std::raise(SIGINT);
	EXPECT_EQ(raiseResult, 0);
	EXPECT_EQ(g_handlerCallCount, 1);
	EXPECT_EQ(g_lastSignal, SIGINT);

	void (*restoreResult)(int) = std::signal(SIGINT, previousHandler);
	ASSERT_NE(restoreResult, SIG_ERR);
}

TEST(CSignal, SigIgnDisablesHandlerDelivery)
{
	// SIG_IGN is the "ignore this signal" disposition. After setting SIGINT to
	// SIG_IGN, raising SIGINT should not invoke our custom handler. We restore the
	// previous handler afterwards to keep the test process behavior unchanged.
	g_handlerCallCount = 0;
	g_lastSignal = 0;

	void (*originalHandler)(int) = std::signal(SIGINT, CountingSignalHandler);
	ASSERT_NE(originalHandler, SIG_ERR);
	void (*previousHandler)(int) = std::signal(SIGINT, SIG_IGN);
	ASSERT_NE(previousHandler, SIG_ERR);

	const int raiseResult = std::raise(SIGINT);
	EXPECT_EQ(raiseResult, 0);
	EXPECT_EQ(g_handlerCallCount, 0);
	EXPECT_EQ(g_lastSignal, 0);

	void (*restoreIgnoredResult)(int) = std::signal(SIGINT, previousHandler);
	ASSERT_NE(restoreIgnoredResult, SIG_ERR);
	void (*restoreOriginalResult)(int) = std::signal(SIGINT, originalHandler);
	ASSERT_NE(restoreOriginalResult, SIG_ERR);
}

TEST(CSignal, SignalDispositionConstantsAreUsable)
{
	// SIG_DFL (default), SIG_IGN (ignore), and SIG_ERR (installation failure)
	// are the required disposition constants in <csignal>. This test does not
	// force failure paths; instead it checks these constants are distinct handles
	// so callers can reliably compare against them.
	EXPECT_NE(SIG_DFL, SIG_IGN);
	EXPECT_NE(SIG_DFL, SIG_ERR);
	EXPECT_NE(SIG_IGN, SIG_ERR);
}

TEST(CSignal, StandardSignalMacrosAreDefined)
{
	// <csignal> provides at least these standard signal-number macros:
	// SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, and SIGTERM.
	// Their exact numeric values are implementation-defined, so the test focuses
	// on existence and basic sanity (non-zero identifiers).
	EXPECT_NE(SIGABRT, 0);
	EXPECT_NE(SIGFPE, 0);
	EXPECT_NE(SIGILL, 0);
	EXPECT_NE(SIGINT, 0);
	EXPECT_NE(SIGSEGV, 0);
	EXPECT_NE(SIGTERM, 0);
}

}  // namespace
