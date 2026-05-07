#include <gtest/gtest.h>

#include <compare>
#include <limits>
#include <type_traits>

namespace {

	TEST(Compare, StrongOrderingConstantsAndRelations)
	{
		// std::strong_ordering models total ordering where values are fully
		// comparable and substitutable for equality. This test validates canonical
		// category constants and verifies relational checks against literal zero,
		// which is the standard way to interrogate comparison category results.
		const auto less = std::strong_ordering::less;
		const auto equal = std::strong_ordering::equal;
		const auto equivalent = std::strong_ordering::equivalent;
		const auto greater = std::strong_ordering::greater;

		EXPECT_TRUE(less < 0);
		EXPECT_TRUE(equal == 0);
		EXPECT_TRUE(equivalent == 0);
		EXPECT_TRUE(greater > 0);
	}

	TEST(Compare, WeakOrderingConstantsAndRelations)
	{
		// std::weak_ordering models total ordering with equivalence classes where
		// equivalent values are not necessarily substitutable. This test validates
		// less/equivalent/greater constants and confirms the same zero-comparison
		// interrogation style used by standard comparison categories.
		const auto less = std::weak_ordering::less;
		const auto equivalent = std::weak_ordering::equivalent;
		const auto greater = std::weak_ordering::greater;

		EXPECT_TRUE(less < 0);
		EXPECT_TRUE(equivalent == 0);
		EXPECT_TRUE(greater > 0);
	}

	TEST(Compare, PartialOrderingConstantsAndUnorderedState)
	{
		// std::partial_ordering supports cases like NaN where comparison can be
		// unordered. This test checks less/equivalent/greater and explicitly verifies
		// that unordered does not compare less/equal/greater to zero.
		const auto less = std::partial_ordering::less;
		const auto equivalent = std::partial_ordering::equivalent;
		const auto greater = std::partial_ordering::greater;
		const auto unordered = std::partial_ordering::unordered;

		EXPECT_TRUE(less < 0);
		EXPECT_TRUE(equivalent == 0);
		EXPECT_TRUE(greater > 0);

		EXPECT_FALSE(unordered < 0);
		EXPECT_FALSE(unordered == 0);
		EXPECT_FALSE(unordered > 0);
	}

	TEST(Compare, ComparisonCategoryHelperPredicates)
	{
		// <compare> provides helper predicates (`is_eq`, `is_neq`, `is_lt`, `is_lteq`,
		// `is_gt`, `is_gteq`) that interpret category results in a readable and
		// generic way. This test validates each helper on representative outcomes,
		// including unordered semantics where equality/ordering predicates are false.
		EXPECT_TRUE(std::is_eq(std::strong_ordering::equal));
		EXPECT_TRUE(std::is_neq(std::strong_ordering::less));
		EXPECT_TRUE(std::is_lt(std::strong_ordering::less));
		EXPECT_TRUE(std::is_lteq(std::strong_ordering::equivalent));
		EXPECT_TRUE(std::is_gt(std::strong_ordering::greater));
		EXPECT_TRUE(std::is_gteq(std::strong_ordering::equivalent));

		EXPECT_FALSE(std::is_eq(std::partial_ordering::unordered));
		EXPECT_TRUE(std::is_neq(std::partial_ordering::unordered));
		EXPECT_FALSE(std::is_lt(std::partial_ordering::unordered));
		EXPECT_FALSE(std::is_gt(std::partial_ordering::unordered));
	}

	TEST(Compare, CompareThreeWayFunctionObject)
	{
		// std::compare_three_way is a transparent function object that invokes <=> on
		// operands and returns the corresponding comparison category type. This test
		// validates usage with integral and floating-point values and confirms expected
		// relation outcomes, including unordered behavior for NaN comparisons.
		const std::compare_three_way cmp{};

		const auto intResult = cmp(3, 7);
		EXPECT_TRUE(intResult < 0);
		EXPECT_TRUE((std::is_same_v<decltype(intResult), const std::strong_ordering>));

		constexpr const double nan = std::numeric_limits<double>::quiet_NaN();
		const auto fpResult = cmp(nan, 1.0);
		EXPECT_TRUE((std::is_same_v<decltype(fpResult), const std::partial_ordering>));
		EXPECT_EQ(fpResult, std::partial_ordering::unordered);
	}

	TEST(Compare, CompareThreeWayResultTypeTrait)
	{
		// std::compare_three_way_result_t<T, U> computes the deduced category type
		// produced by three-way comparison between T and U. This test checks common
		// built-in cases to ensure trait behavior matches language comparison rules.
		EXPECT_TRUE((std::is_same_v<std::compare_three_way_result_t<int, int>, std::strong_ordering>));
		EXPECT_TRUE((std::is_same_v<std::compare_three_way_result_t<double, double>, std::partial_ordering>));
	}

	TEST(Compare, CommonComparisonCategoryTypeTrait)
	{
		// std::common_comparison_category_t computes the "weakest" category capable
		// representing all listed comparison results. This test verifies key lattice
		// combinations, demonstrating promotion from strong->weak->partial as needed.
		EXPECT_TRUE((std::is_same_v<
			std::common_comparison_category_t<std::strong_ordering, std::strong_ordering>,
			std::strong_ordering>));

		EXPECT_TRUE((std::is_same_v<
			std::common_comparison_category_t<std::strong_ordering, std::weak_ordering>,
			std::weak_ordering>));

		EXPECT_TRUE((std::is_same_v<
			std::common_comparison_category_t<std::strong_ordering, std::partial_ordering>,
			std::partial_ordering>));
	}

	TEST(Compare, ComparisonConceptsForBuiltInTypes)
	{
		// <compare> defines concepts that classify types by the strength of their
		// three-way comparison support. This test validates that integers satisfy
		// strong comparability and floating-point values satisfy partial comparability
		// due to NaN-induced unordered results.
		EXPECT_TRUE((std::three_way_comparable<int, std::strong_ordering>));
		EXPECT_TRUE((std::three_way_comparable<double, std::partial_ordering>));
		EXPECT_FALSE((std::three_way_comparable<double, std::strong_ordering>));
	}

}  // namespace
