#include <gtest/gtest.h>

#include <csetjmp>

namespace {

[[noreturn]] void JumpWithCode(std::jmp_buf env, int code)
{
	// longjmp performs a non-local transfer to the most recent active setjmp
	// point stored in `env`. The function is marked [[noreturn]] because control
	// never returns to this call site after longjmp succeeds.
	std::longjmp(env, code);
}

TEST(CSetJmp, JmpBufStoresExecutionEnvironment)
{
	// jmp_buf is the opaque storage type used by setjmp/longjmp to capture and
	// later restore execution state. This test validates that the type is usable
	// in regular C++ code by declaring a buffer and invoking setjmp on it.
	std::jmp_buf env{};
	const int result = setjmp(env);

	// The first return from setjmp always yields zero because no jump occurred.
	// A non-zero value would mean we resumed here through longjmp.
	EXPECT_EQ(result, 0);
}

TEST(CSetJmp, SetjmpAndLongjmpRoundTripValue)
{
	// setjmp captures the current continuation and returns 0 initially. A later
	// longjmp to the same buffer causes setjmp to return the supplied non-zero
	// code, allowing structured error-style unwinding in C APIs.
	std::jmp_buf env{};
	volatile int enteredViaLongjmp = 0;
	const int result = setjmp(env);

	if (result == 0)
	{
		// First pass: trigger a non-local jump back to the saved point.
		JumpWithCode(env, 7);
	}
	else
	{
		enteredViaLongjmp = 1;
	}

	EXPECT_EQ(result, 7);
	EXPECT_EQ(enteredViaLongjmp, 1);
}

TEST(CSetJmp, LongjmpWithZeroBecomesOne)
{
	// The C/C++ standard specifies that longjmp(..., 0) is adjusted so setjmp
	// returns 1 instead of 0. This prevents ambiguity with the initial setjmp
	// return path, which is always exactly 0.
	std::jmp_buf env{};
	const int result = setjmp(env);

	if (result == 0)
	{
		JumpWithCode(env, 0);
	}

	EXPECT_EQ(result, 1);
}

}  // namespace
