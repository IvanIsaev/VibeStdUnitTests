#include <gtest/gtest.h>

#include <cmath>
#include <limits>

namespace {

TEST(CMathHeader, BasicArithmeticFunctions)
{
	// Core transcendental and power functions are defined in <cmath>.
	EXPECT_DOUBLE_EQ(std::sqrt(49.0), 7.0);
	EXPECT_DOUBLE_EQ(std::pow(2.0, 5.0), 32.0);
	EXPECT_DOUBLE_EQ(std::abs(-3.5), 3.5);
	EXPECT_NEAR(std::sin(0.0), 0.0, 1e-12);
	EXPECT_NEAR(std::cos(0.0), 1.0, 1e-12);
}

TEST(CMathHeader, RoundingAndRemainderFamilies)
{
	// Rounding functions expose different semantics for midpoint handling.
	EXPECT_DOUBLE_EQ(std::floor(3.9), 3.0);
	EXPECT_DOUBLE_EQ(std::ceil(3.1), 4.0);
	EXPECT_DOUBLE_EQ(std::trunc(-3.9), -3.0);
	EXPECT_DOUBLE_EQ(std::round(2.5), 3.0);
	EXPECT_DOUBLE_EQ(std::fmod(7.5, 2.0), 1.5);
}

TEST(CMathHeader, ClassificationAndComparisonUtilities)
{
	// Classification helpers detect finite/infinite/NaN values portably.
	const double inf = std::numeric_limits<double>::infinity();
	const double nan = std::numeric_limits<double>::quiet_NaN();
	EXPECT_TRUE(std::isfinite(1.0));
	EXPECT_TRUE(std::isinf(inf));
	EXPECT_TRUE(std::isnan(nan));
	EXPECT_TRUE(std::isnormal(1.0));
	EXPECT_TRUE(std::signbit(-0.0));
}

TEST(CMathHeader, ExponentialAndLogarithmicFunctions)
{
	// Exponential/logarithmic families are inverses under ideal precision.
	EXPECT_NEAR(std::exp(1.0), 2.718281828459045, 1e-12);
	EXPECT_NEAR(std::log(std::exp(2.0)), 2.0, 1e-12);
	EXPECT_NEAR(std::log10(1000.0), 3.0, 1e-12);
	EXPECT_NEAR(std::log2(8.0), 3.0, 1e-12);
}

}  // namespace
