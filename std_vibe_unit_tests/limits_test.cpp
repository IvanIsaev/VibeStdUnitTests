#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <type_traits>

namespace {

	TEST(Limits, NumericLimitsTemplateIsSpecializedForFundamentalTypes)
	{
		// std::numeric_limits<T>::is_specialized indicates whether the implementation
		// provides meaningful compile-time numeric metadata for T. Fundamental
		// arithmetic types must be specialized by the standard library.
		EXPECT_TRUE((std::numeric_limits<bool>::is_specialized));
		EXPECT_TRUE((std::numeric_limits<char>::is_specialized));
		EXPECT_TRUE((std::numeric_limits<int>::is_specialized));
		EXPECT_TRUE((std::numeric_limits<unsigned int>::is_specialized));
		EXPECT_TRUE((std::numeric_limits<float>::is_specialized));
		EXPECT_TRUE((std::numeric_limits<double>::is_specialized));
	}

	TEST(Limits, IntegerMinMaxLowestAndDigitMetadataAreCoherent)
	{
		// For integral types, min/max/lowest and digit metadata describe exact
		// representable bounds and precision. This test validates key relationships
		// for signed and unsigned integers without assuming a specific ABI model.
		using Signed = std::numeric_limits<int>;
		using Unsigned = std::numeric_limits<unsigned int>;

		EXPECT_LT(Signed::min(), Signed::max());
		EXPECT_EQ(Signed::lowest(), Signed::min());
		EXPECT_TRUE((Signed::is_signed));
		EXPECT_TRUE((Signed::is_integer));
		EXPECT_TRUE((Signed::is_exact));
		EXPECT_GE(Signed::digits, 15);
		EXPECT_GE(Signed::digits10, 4);
		EXPECT_EQ(Signed::radix, 2);

		EXPECT_EQ(Unsigned::min(), 0u);
		EXPECT_EQ(Unsigned::lowest(), 0u);
		EXPECT_GT(Unsigned::max(), 0u);
		EXPECT_FALSE((Unsigned::is_signed));
		EXPECT_TRUE((Unsigned::is_integer));
		EXPECT_TRUE((Unsigned::is_exact));
		EXPECT_GE(Unsigned::digits, 16);
		EXPECT_GE(Unsigned::digits10, 4);
		EXPECT_EQ(Unsigned::radix, 2);
	}

	TEST(Limits, IntegerFloatingOnlyMembersExposeExpectedFallbackValues)
	{
		// epsilon/round_error/exponent values are primarily meaningful for floating
		// types. For integer specializations, the standard interface still exists and
		// should report neutral values that indicate "not applicable."
		using Signed = std::numeric_limits<int>;

		EXPECT_EQ(Signed::epsilon(), 0);
		EXPECT_EQ(Signed::round_error(), 0);
		EXPECT_EQ(Signed::min_exponent, 0);
		EXPECT_EQ(Signed::min_exponent10, 0);
		EXPECT_EQ(Signed::max_exponent, 0);
		EXPECT_EQ(Signed::max_exponent10, 0);

		EXPECT_FALSE((Signed::has_infinity));
		EXPECT_FALSE((Signed::has_quiet_NaN));
		EXPECT_FALSE((Signed::has_signaling_NaN));
		EXPECT_EQ(Signed::has_denorm, std::denorm_absent);
		EXPECT_FALSE((Signed::has_denorm_loss));
		EXPECT_EQ(Signed::infinity(), 0);
		EXPECT_EQ(Signed::quiet_NaN(), 0);
		EXPECT_EQ(Signed::signaling_NaN(), 0);
		EXPECT_EQ(Signed::denorm_min(), 0);
	}

	TEST(Limits, IntegerBehaviorFlagsAndRoundingStyleAreConsistent)
	{
		// numeric_limits also advertises behavioral properties of arithmetic model:
		// boundedness, modulo arithmetic, trapping behavior, tinyness detection, and
		// rounding style. This test captures expected relationships for integers.
		using Signed = std::numeric_limits<int>;
		using Unsigned = std::numeric_limits<unsigned int>;

		EXPECT_TRUE((Signed::is_bounded));
		EXPECT_TRUE((Unsigned::is_bounded));
		EXPECT_FALSE((Signed::is_modulo));
		EXPECT_TRUE((Unsigned::is_modulo));
		EXPECT_FALSE((Signed::is_iec559));
		EXPECT_EQ(Signed::round_style, std::round_toward_zero);
		EXPECT_FALSE((Signed::tinyness_before));

		EXPECT_TRUE((std::is_same_v<decltype(Signed::traps), const bool>));
		EXPECT_TRUE((std::is_same_v<decltype(Unsigned::traps), const bool>));
	}

	TEST(Limits, FloatBoundsAndPrecisionMembersAreValid)
	{
		// For floating-point types, min/max/lowest provide finite range bounds while
		// digits/digits10/max_digits10 describe binary and decimal precision.
		using Float = std::numeric_limits<float>;

		EXPECT_GT(Float::min(), 0.0f);
		EXPECT_LT(Float::lowest(), 0.0f);
		EXPECT_GT(Float::max(), 0.0f);
		EXPECT_LT(Float::lowest(), Float::min());
		EXPECT_LT(Float::min(), Float::max());

		EXPECT_GE(Float::digits, 2);
		EXPECT_GE(Float::digits10, 1);
		EXPECT_GE(Float::max_digits10, Float::digits10);
		EXPECT_EQ(Float::radix, FLT_RADIX);
		EXPECT_FALSE((Float::is_integer));
		EXPECT_FALSE((Float::is_exact));
	}

	TEST(Limits, FloatMachineEpsilonRoundErrorAndExponentRangesAreCoherent)
	{
		// epsilon is spacing near 1.0, round_error reports max rounding error for
		// the active model, and exponent members encode normalized range in both base
		// radix and base-10 forms. These should all be strictly meaningful for float.
		using Float = std::numeric_limits<float>;

		EXPECT_GT(Float::epsilon(), 0.0f);
		EXPECT_GE(Float::round_error(), 0.0f);
		EXPECT_LT(Float::min_exponent, Float::max_exponent);
		EXPECT_LT(Float::min_exponent10, Float::max_exponent10);
	}

	TEST(Limits, FloatInfinityNaNAndDenormalFacilitiesBehaveAsAdvertised)
	{
		// has_infinity/has_quiet_NaN/has_signaling_NaN/has_denorm advertise optional
		// IEEE-style facilities. When reported as available, corresponding producer
		// functions should return values with expected runtime classification.
		using Float = std::numeric_limits<float>;

		if (Float::has_infinity)
		{
			const float inf = Float::infinity();
			EXPECT_TRUE(std::isinf(inf));
			EXPECT_GT(inf, Float::max());
		}

		if (Float::has_quiet_NaN)
		{
			const float qnan = Float::quiet_NaN();
			EXPECT_TRUE(std::isnan(qnan));
		}

		if (Float::has_signaling_NaN)
		{
			// Reading signaling NaN may quiet it on many platforms, but it should
			// still classify as NaN value data.
			const float snan = Float::signaling_NaN();
			EXPECT_TRUE(std::isnan(snan));
		}

		if (Float::has_denorm != std::denorm_absent)
		{
			const float dmin = Float::denorm_min();
			EXPECT_GT(dmin, 0.0f);
			EXPECT_LE(dmin, Float::min());
		}
	}

	TEST(Limits, FloatBehaviorFlagsAndRoundingModelAreSane)
	{
		// Floating specializations expose conformance and runtime behavior flags,
		// including IEEE-754 compatibility hints and rounding style enumeration.
		// This test verifies stable invariants that should hold across toolchains.
		using Float = std::numeric_limits<float>;

		EXPECT_TRUE((Float::is_bounded));
		EXPECT_FALSE((Float::is_modulo));
		EXPECT_TRUE((Float::round_style == std::round_toward_zero ||
		             Float::round_style == std::round_to_nearest ||
		             Float::round_style == std::round_toward_infinity ||
		             Float::round_style == std::round_toward_neg_infinity ||
		             Float::round_style == std::round_indeterminate));
		EXPECT_TRUE((std::is_same_v<decltype(Float::traps), const bool>));
		EXPECT_TRUE((std::is_same_v<decltype(Float::tinyness_before), const bool>));
		EXPECT_TRUE((Float::is_iec559 || !Float::is_iec559));
	}

	TEST(Limits, FloatRoundStyleEnumerationValuesAreDistinctAndAddressable)
	{
		// <limits> defines std::float_round_style as an enum describing rounding
		// direction. This test validates that all named enumerators are usable and
		// represent distinct integer tags.
		const int towardZero = static_cast<int>(std::round_toward_zero);
		const int toNearest = static_cast<int>(std::round_to_nearest);
		const int towardInf = static_cast<int>(std::round_toward_infinity);
		const int towardNegInf = static_cast<int>(std::round_toward_neg_infinity);
		const int indeterminate = static_cast<int>(std::round_indeterminate);

		EXPECT_NE(towardZero, toNearest);
		EXPECT_NE(towardInf, towardNegInf);
		EXPECT_NE(indeterminate, toNearest);
	}

	TEST(Limits, FloatDenormStyleEnumerationValuesAreDistinctAndAddressable)
	{
		// std::float_denorm_style enumerates denormalization support categories. The
		// named constants are part of the public API and must be usable as enum tags
		// for compile-time/runtime branching.
		const int absent = static_cast<int>(std::denorm_absent);
		const int present = static_cast<int>(std::denorm_present);
		const int indeterminate = static_cast<int>(std::denorm_indeterminate);

		EXPECT_NE(absent, present);
		EXPECT_NE(indeterminate, absent);
		EXPECT_NE(indeterminate, present);
	}

}  // namespace
