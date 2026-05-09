#include <gtest/gtest.h>

#include <ratio>
#include <type_traits>

namespace {

	TEST(Ratio, RatioTypeNormalizesNumeratorAndDenominator)
	{
		// std::ratio<N, D> represents a compile-time rational constant and always
		// normalizes sign and greatest-common-divisor. The denominator is positive,
		// and num/den are reduced to canonical form.
		using R1 = std::ratio<6, 8>;
		using R2 = std::ratio<-10, -20>;
		using R3 = std::ratio<5, -15>;

		EXPECT_EQ(R1::num, 3);
		EXPECT_EQ(R1::den, 4);

		EXPECT_EQ(R2::num, 1);
		EXPECT_EQ(R2::den, 2);

		EXPECT_EQ(R3::num, -1);
		EXPECT_EQ(R3::den, 3);
	}

	TEST(Ratio, RatioAddSubtractMultiplyDivideProduceExpectedCanonicalResults)
	{
		// ratio_add/subtract/multiply/divide perform compile-time rational arithmetic
		// and return normalized std::ratio result types.
		using A = std::ratio<1, 3>;
		using B = std::ratio<1, 6>;

		using Sum = std::ratio_add<A, B>;
		using Diff = std::ratio_subtract<A, B>;
		using Product = std::ratio_multiply<A, B>;
		using Quotient = std::ratio_divide<A, B>;

		EXPECT_EQ(Sum::num, 1);
		EXPECT_EQ(Sum::den, 2);

		EXPECT_EQ(Diff::num, 1);
		EXPECT_EQ(Diff::den, 6);

		EXPECT_EQ(Product::num, 1);
		EXPECT_EQ(Product::den, 18);

		EXPECT_EQ(Quotient::num, 2);
		EXPECT_EQ(Quotient::den, 1);
	}

	TEST(Ratio, RatioComparisonMetafunctionsAndValueHelpersAreConsistent)
	{
		// <ratio> provides both type-based comparison metafunctions and *_v boolean
		// helpers. This test checks all ordering/equality relations for sample ratios.
		using Small = std::ratio<2, 5>;
		using EqualToSmall = std::ratio<4, 10>;
		using Large = std::ratio<7, 10>;

		EXPECT_TRUE((std::ratio_equal<Small, EqualToSmall>::value));
		EXPECT_TRUE((std::ratio_equal_v<Small, EqualToSmall>));
		EXPECT_FALSE((std::ratio_not_equal_v<Small, EqualToSmall>));

		EXPECT_TRUE((std::ratio_less<Small, Large>::value));
		EXPECT_TRUE((std::ratio_less_v<Small, Large>));
		EXPECT_TRUE((std::ratio_less_equal_v<Small, EqualToSmall>));

		EXPECT_TRUE((std::ratio_greater<Large, Small>::value));
		EXPECT_TRUE((std::ratio_greater_v<Large, Small>));
		EXPECT_TRUE((std::ratio_greater_equal_v<Large, Small>));
	}

	TEST(Ratio, PredefinedDecimalScaleRatiosMatchExpectedPowersOfTen)
	{
		// The decimal SI typedefs in <ratio> encode powers of ten commonly used by
		// duration, chrono, and unit libraries.
		EXPECT_EQ(std::atto::num, 1);
		EXPECT_EQ(std::atto::den, 1000000000000000000LL);

		EXPECT_EQ(std::femto::num, 1);
		EXPECT_EQ(std::femto::den, 1000000000000000LL);

		EXPECT_EQ(std::pico::num, 1);
		EXPECT_EQ(std::pico::den, 1000000000000LL);

		EXPECT_EQ(std::nano::num, 1);
		EXPECT_EQ(std::nano::den, 1000000000LL);

		EXPECT_EQ(std::micro::num, 1);
		EXPECT_EQ(std::micro::den, 1000000LL);

		EXPECT_EQ(std::milli::num, 1);
		EXPECT_EQ(std::milli::den, 1000LL);

		EXPECT_EQ(std::centi::num, 1);
		EXPECT_EQ(std::centi::den, 100LL);

		EXPECT_EQ(std::deci::num, 1);
		EXPECT_EQ(std::deci::den, 10LL);

		EXPECT_EQ(std::deca::num, 10LL);
		EXPECT_EQ(std::deca::den, 1);

		EXPECT_EQ(std::hecto::num, 100LL);
		EXPECT_EQ(std::hecto::den, 1);

		EXPECT_EQ(std::kilo::num, 1000LL);
		EXPECT_EQ(std::kilo::den, 1);

		EXPECT_EQ(std::mega::num, 1000000LL);
		EXPECT_EQ(std::mega::den, 1);

		EXPECT_EQ(std::giga::num, 1000000000LL);
		EXPECT_EQ(std::giga::den, 1);

		EXPECT_EQ(std::tera::num, 1000000000000LL);
		EXPECT_EQ(std::tera::den, 1);

		EXPECT_EQ(std::peta::num, 1000000000000000LL);
		EXPECT_EQ(std::peta::den, 1);

		EXPECT_EQ(std::exa::num, 1000000000000000000LL);
		EXPECT_EQ(std::exa::den, 1);
	}

	TEST(Ratio, RatioTypeAliasesAreProperRatioSpecializations)
	{
		// Predefined aliases are exact std::ratio instantiations and can be compared
		// structurally via type identity checks.
		EXPECT_TRUE((std::is_same_v<std::kilo, std::ratio<1000, 1>>));
		EXPECT_TRUE((std::is_same_v<std::milli, std::ratio<1, 1000>>));
		EXPECT_TRUE((std::is_same_v<std::mega, std::ratio<1000000, 1>>));
		EXPECT_TRUE((std::is_same_v<std::micro, std::ratio<1, 1000000>>));
	}

}  // namespace
