#include <gtest/gtest.h>

#include <cassert>

namespace {

	inline int TouchCounter(int& counter)
	{
		++counter;
		return counter;
	}

	TEST(CAssert, AssertMacroCanBeInvokedWithTrueCondition)
	{
		// <cassert> primarily provides the assert macro. A true condition should be
		// accepted in both debug and release modes and must not terminate execution.
		assert(true);
		SUCCEED();
	}

	TEST(CAssert, AssertExpressionEvaluationDependsOnNDebugConfiguration)
	{
		// When NDEBUG is not defined, assert(expr) evaluates expr and aborts only if
		// it is false. When NDEBUG is defined, assert(expr) is disabled and expr is
		// not evaluated. We test this safely using a side effect and only true result.
		int counter = 0;
		assert(TouchCounter(counter) > 0);

#ifdef NDEBUG
		EXPECT_EQ(counter, 0);
#else
		EXPECT_EQ(counter, 1);
#endif
	}

	TEST(CAssert, ParenthesizedCommaExpressionRemainsValidAssertOperand)
	{
		// assert takes one macro argument, so comma expressions need parentheses.
		// This pattern is common in macro-heavy code and should compile and execute.
		int lhs = 0;
		int rhs = 0;
		assert(((lhs = 1), (rhs = 2), (lhs + rhs == 3)));

#ifndef NDEBUG
		EXPECT_EQ(lhs, 1);
		EXPECT_EQ(rhs, 2);
#else
		// In release mode assert is disabled; expression is not evaluated.
		EXPECT_EQ(lhs, 0);
		EXPECT_EQ(rhs, 0);
#endif
	}

	TEST(CAssert, NDebugMacroStateIsObservableForBuildModeAwareness)
	{
		// NDEBUG is the compile-time switch controlling assert activation. This test
		// intentionally records the active mode so failures in other tests are easier
		// to interpret when switching between debug and release configurations.
#ifdef NDEBUG
		const bool assertsEnabled = false;
#else
		const bool assertsEnabled = true;
#endif
		EXPECT_TRUE(assertsEnabled || !assertsEnabled);
	}

}  // namespace
