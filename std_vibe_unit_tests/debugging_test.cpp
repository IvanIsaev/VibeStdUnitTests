#include <gtest/gtest.h>

#include <type_traits>

#if defined(__has_include)
#if __has_include(<debugging>)
#include <debugging>
#define VIBE_HAS_DEBUGGING_HEADER 1
#else
#define VIBE_HAS_DEBUGGING_HEADER 0
#endif
#else
#define VIBE_HAS_DEBUGGING_HEADER 0
#endif

namespace {

	TEST(DebuggingHeader, HeaderAvailabilityAndFeatureMacroContract)
	{
		// <debugging> is a C++26-facing header and may not yet be available on all
		// standard library implementations. This test keeps the suite portable while
		// validating feature macro presence when the header exists.
#if VIBE_HAS_DEBUGGING_HEADER
#ifdef __cpp_lib_debugging
		EXPECT_GE(__cpp_lib_debugging, 202311L);
#else
		FAIL() << "<debugging> is present but __cpp_lib_debugging is missing.";
#endif
#else
		GTEST_SKIP() << "<debugging> is not available on this toolchain.";
#endif
	}

#if VIBE_HAS_DEBUGGING_HEADER

	TEST(DebuggingHeader, IsDebuggerPresentReturnsBooleanAndIsNoexcept)
	{
		// std::is_debugger_present reports whether a debugger is currently attached.
		// The function is intended for low-overhead diagnostic branching and should
		// be callable in noexcept contexts, returning a bool status.
		EXPECT_TRUE((std::is_same_v<decltype(std::is_debugger_present()), bool>));
		EXPECT_TRUE(noexcept(std::is_debugger_present()));

		const bool attached = std::is_debugger_present();
		EXPECT_TRUE(attached || !attached);
	}

	TEST(DebuggingHeader, BreakpointIfDebuggingIsCallableAndNoexcept)
	{
		// std::breakpoint_if_debugging triggers a breakpoint only when a debugger is
		// attached. We verify API shape and noexcept contract. Runtime invocation is
		// intentionally avoided in automated tests to prevent debugger-dependent traps.
		EXPECT_TRUE((std::is_same_v<decltype(std::breakpoint_if_debugging()), void>));
		EXPECT_TRUE(noexcept(std::breakpoint_if_debugging()));
		SUCCEED();
	}

	TEST(DebuggingHeader, BreakpointFunctionExistsWithExpectedSignature)
	{
		// std::breakpoint is an unconditional debug trap primitive. Invoking it in
		// unit tests would intentionally interrupt execution, so this test validates
		// existence/signature/noexcept properties without calling it.
		EXPECT_TRUE((std::is_same_v<decltype(std::breakpoint()), void>));
		EXPECT_TRUE(noexcept(std::breakpoint()));
	}

#endif  // VIBE_HAS_DEBUGGING_HEADER

}  // namespace
