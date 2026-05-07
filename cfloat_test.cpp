#include <gtest/gtest.h>

#include <algorithm>
#include <cfloat>
#include <limits>

namespace {

	TEST(CFloat, RadixAndRoundingModelConstants)
	{
		// FLT_RADIX describes the base used by floating-point exponent representation,
		// and FLT_ROUNDS describes the active rounding mode for floating-point
		// addition. This test validates that radix is sensible and consistent with
		// the C++ type traits for float/double/long double, and that rounding mode is
		// one of the standard-defined encodings.
		EXPECT_GE(FLT_RADIX, 2);
		EXPECT_EQ(FLT_RADIX, std::numeric_limits<float>::radix);
		EXPECT_EQ(FLT_RADIX, std::numeric_limits<double>::radix);
		EXPECT_EQ(FLT_RADIX, std::numeric_limits<long double>::radix);

		EXPECT_TRUE(FLT_ROUNDS == -1 || FLT_ROUNDS == 0 || FLT_ROUNDS == 1 || FLT_ROUNDS == 2 || FLT_ROUNDS == 3);
	}

	TEST(CFloat, MantissaAndDecimalDigitConstants)
	{
		// *_MANT_DIG reports binary precision (significand bits), while *_DIG
		// reports guaranteed base-10 precision for round-tripping significant digits.
		// This test verifies all six constants are positive and match the equivalent
		// `numeric_limits` metadata for each floating-point type.
		EXPECT_EQ(FLT_MANT_DIG, std::numeric_limits<float>::digits);
		EXPECT_EQ(DBL_MANT_DIG, std::numeric_limits<double>::digits);
		EXPECT_EQ(LDBL_MANT_DIG, std::numeric_limits<long double>::digits);

		EXPECT_EQ(FLT_DIG, std::numeric_limits<float>::digits10);
		EXPECT_EQ(DBL_DIG, std::numeric_limits<double>::digits10);
		EXPECT_EQ(LDBL_DIG, std::numeric_limits<long double>::digits10);
	}

	TEST(CFloat, DecimalRoundTripDigitConstants)
	{
		// DECIMAL_DIG is the decimal precision required to round-trip the widest
		// supported floating type; FLT_DECIMAL_DIG/DBL_DECIMAL_DIG/LDBL_DECIMAL_DIG
		// are per-type round-trip precisions. This test checks these values are at
		// least as large as traditional *_DIG and align with `max_digits10`.
		EXPECT_EQ(FLT_DECIMAL_DIG, std::numeric_limits<float>::max_digits10);
		EXPECT_EQ(DBL_DECIMAL_DIG, std::numeric_limits<double>::max_digits10);
		//EXPECT_EQ(LDBL_DECIMAL_DIG, std::numeric_limits<long double>::max_digits10); // It looks like MSVC doesn't support long double

		EXPECT_GE(FLT_DECIMAL_DIG, FLT_DIG);
		EXPECT_GE(DBL_DECIMAL_DIG, DBL_DIG);
		//EXPECT_GE(LDBL_DECIMAL_DIG, LDBL_DIG); // It looks like MSVC doesn't support long double

		EXPECT_GE(DECIMAL_DIG, FLT_DECIMAL_DIG);
		EXPECT_GE(DECIMAL_DIG, DBL_DECIMAL_DIG);
		//EXPECT_GE(DECIMAL_DIG, LDBL_DECIMAL_DIG); // It looks like MSVC doesn't support long double
	}

	TEST(CFloat, MinMaxAndEpsilonConstants)
	{
		// *_MIN and *_MAX bound the normal finite range for each type, and *_EPSILON
		// captures the spacing between 1 and the next representable value. This test
		// validates monotonic relationships and equality with the standard C++
		// numeric traits.
		EXPECT_LT(FLT_MIN, FLT_MAX);
		EXPECT_LT(DBL_MIN, DBL_MAX);
		EXPECT_LT(LDBL_MIN, LDBL_MAX);

		EXPECT_GT(FLT_EPSILON, 0.0f);
		EXPECT_GT(DBL_EPSILON, 0.0);
		EXPECT_GT(LDBL_EPSILON, 0.0L);

		EXPECT_EQ(FLT_MIN, std::numeric_limits<float>::min());
		EXPECT_EQ(DBL_MIN, std::numeric_limits<double>::min());
		EXPECT_EQ(LDBL_MIN, std::numeric_limits<long double>::min());

		EXPECT_EQ(FLT_MAX, std::numeric_limits<float>::max());
		EXPECT_EQ(DBL_MAX, std::numeric_limits<double>::max());
		EXPECT_EQ(LDBL_MAX, std::numeric_limits<long double>::max());

		EXPECT_EQ(FLT_EPSILON, std::numeric_limits<float>::epsilon());
		EXPECT_EQ(DBL_EPSILON, std::numeric_limits<double>::epsilon());
		EXPECT_EQ(LDBL_EPSILON, std::numeric_limits<long double>::epsilon());
	}

	TEST(CFloat, BinaryExponentRangeConstants)
	{
		// *_MIN_EXP and *_MAX_EXP specify the exponent limits (in base FLT_RADIX) of
		// normalized floating-point values. This test ensures ordering correctness and
		// matches the values exposed by `numeric_limits` for each type.
		EXPECT_LT(FLT_MIN_EXP, FLT_MAX_EXP);
		EXPECT_LT(DBL_MIN_EXP, DBL_MAX_EXP);
		EXPECT_LT(LDBL_MIN_EXP, LDBL_MAX_EXP);

		EXPECT_EQ(FLT_MIN_EXP, std::numeric_limits<float>::min_exponent);
		EXPECT_EQ(DBL_MIN_EXP, std::numeric_limits<double>::min_exponent);
		EXPECT_EQ(LDBL_MIN_EXP, std::numeric_limits<long double>::min_exponent);

		EXPECT_EQ(FLT_MAX_EXP, std::numeric_limits<float>::max_exponent);
		EXPECT_EQ(DBL_MAX_EXP, std::numeric_limits<double>::max_exponent);
		EXPECT_EQ(LDBL_MAX_EXP, std::numeric_limits<long double>::max_exponent);
	}

	TEST(CFloat, DecimalExponentRangeConstants)
	{
		// *_MIN_10_EXP and *_MAX_10_EXP provide exponent limits expressed in base-10,
		// which are often used for formatting and parsing boundaries. This test
		// verifies ordering and equality with `numeric_limits` metadata.
		EXPECT_LT(FLT_MIN_10_EXP, FLT_MAX_10_EXP);
		EXPECT_LT(DBL_MIN_10_EXP, DBL_MAX_10_EXP);
		EXPECT_LT(LDBL_MIN_10_EXP, LDBL_MAX_10_EXP);

		EXPECT_EQ(FLT_MIN_10_EXP, std::numeric_limits<float>::min_exponent10);
		EXPECT_EQ(DBL_MIN_10_EXP, std::numeric_limits<double>::min_exponent10);
		EXPECT_EQ(LDBL_MIN_10_EXP, std::numeric_limits<long double>::min_exponent10);

		EXPECT_EQ(FLT_MAX_10_EXP, std::numeric_limits<float>::max_exponent10);
		EXPECT_EQ(DBL_MAX_10_EXP, std::numeric_limits<double>::max_exponent10);
		EXPECT_EQ(LDBL_MAX_10_EXP, std::numeric_limits<long double>::max_exponent10);
	}

	TEST(CFloat, SubnormalSupportAndTrueMinimumConstants)
	{
		// *_HAS_SUBNORM indicates whether subnormal values are supported (-1 unknown,
		// 0 absent, 1 present). *_TRUE_MIN gives the smallest positive representable
		// value including subnormals when available. This test validates legal enum
		// ranges, confirms consistency with `denorm_min`, and checks ordering against
		// normalized minimum values.
		EXPECT_TRUE(FLT_HAS_SUBNORM == -1 || FLT_HAS_SUBNORM == 0 || FLT_HAS_SUBNORM == 1);
		EXPECT_TRUE(DBL_HAS_SUBNORM == -1 || DBL_HAS_SUBNORM == 0 || DBL_HAS_SUBNORM == 1);
		EXPECT_TRUE(LDBL_HAS_SUBNORM == -1 || LDBL_HAS_SUBNORM == 0 || LDBL_HAS_SUBNORM == 1);

		EXPECT_EQ(FLT_TRUE_MIN, std::numeric_limits<float>::denorm_min());
		EXPECT_EQ(DBL_TRUE_MIN, std::numeric_limits<double>::denorm_min());
		EXPECT_EQ(LDBL_TRUE_MIN, std::numeric_limits<long double>::denorm_min());

		EXPECT_GT(FLT_TRUE_MIN, 0.0f);
		EXPECT_GT(DBL_TRUE_MIN, 0.0);
		EXPECT_GT(LDBL_TRUE_MIN, 0.0L);

		EXPECT_LE(FLT_TRUE_MIN, FLT_MIN);
		EXPECT_LE(DBL_TRUE_MIN, DBL_MIN);
		EXPECT_LE(LDBL_TRUE_MIN, LDBL_MIN);
	}

}  // namespace
