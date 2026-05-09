#include <gtest/gtest.h>

#include <numbers>

namespace {

TEST(NumbersHeader, MathematicalConstantsForDouble)
{
	// <numbers> exposes compile-time constants for common mathematical values.
	EXPECT_NEAR(std::numbers::e, 2.718281828459045, 1e-15);
	EXPECT_NEAR(std::numbers::log2e, 1.4426950408889634, 1e-15);
	EXPECT_NEAR(std::numbers::pi, 3.1415926535897932, 1e-15);
	EXPECT_NEAR(std::numbers::inv_pi, 0.3183098861837907, 1e-15);
	EXPECT_NEAR(std::numbers::sqrt2, 1.4142135623730951, 1e-15);
}

TEST(NumbersHeader, FloatTemplateConstants)
{
	// Constants are available as variable templates for multiple float types.
	const float piF = std::numbers::pi_v<float>;
	const long double piLD = std::numbers::pi_v<long double>;
	EXPECT_NEAR(piF, 3.1415927f, 1e-6f);
	EXPECT_NEAR(static_cast<double>(piLD), 3.1415926535897932, 1e-12);
}

}  // namespace
