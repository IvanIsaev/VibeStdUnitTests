#include <gtest/gtest.h>

#include <cfenv>
#include <cmath>

#pragma STDC FENV_ACCESS ON

namespace {

TEST(CFenvHeader, RoundingModeControlAndQuery)
{
	// <cfenv> provides control over floating-point environment flags and modes.
	const int original = std::fegetround();
	ASSERT_NE(original, -1);

	EXPECT_EQ(std::fesetround(FE_DOWNWARD), 0);
	EXPECT_EQ(std::fegetround(), FE_DOWNWARD);

	EXPECT_EQ(std::fesetround(original), 0);
	EXPECT_EQ(std::fegetround(), original);
}

TEST(CFenvHeader, ExceptionFlagsRaiseTestAndClear)
{
	// Floating-point exceptions can be explicitly raised, queried, and cleared.
	EXPECT_EQ(std::feclearexcept(FE_ALL_EXCEPT), 0);
	EXPECT_EQ(std::feraiseexcept(FE_INVALID | FE_DIVBYZERO), 0);
	EXPECT_TRUE(std::fetestexcept(FE_INVALID));
	EXPECT_TRUE(std::fetestexcept(FE_DIVBYZERO));

	EXPECT_EQ(std::feclearexcept(FE_ALL_EXCEPT), 0);
	EXPECT_EQ(std::fetestexcept(FE_ALL_EXCEPT), 0);
}

TEST(CFenvHeader, EnvironmentSaveAndRestore)
{
	// fegetenv/fesetenv snapshot and restore complete floating-point environment.
	fenv_t env{};
	EXPECT_EQ(std::fegetenv(&env), 0);

	EXPECT_EQ(std::feraiseexcept(FE_OVERFLOW), 0);
	EXPECT_TRUE(std::fetestexcept(FE_OVERFLOW));

	EXPECT_EQ(std::fesetenv(&env), 0);
}

}  // namespace
