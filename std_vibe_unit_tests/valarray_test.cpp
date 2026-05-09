#include <gtest/gtest.h>

#include <valarray>

namespace {

TEST(ValarrayHeader, ConstructionAndElementwiseArithmetic)
{
	// valarray performs element-wise numeric operations by default.
	std::valarray<int> a{ 1, 2, 3 };
	std::valarray<int> b{ 4, 5, 6 };
	std::valarray<int> c = a + b;
	EXPECT_EQ(c[0], 5);
	EXPECT_EQ(c[1], 7);
	EXPECT_EQ(c[2], 9);
}

TEST(ValarrayHeader, AggregateFunctionsAndTransforms)
{
	// sum/min/max and apply provide common aggregate/transform workflows.
	std::valarray<double> v{ 1.0, 2.0, 3.0, 4.0 };
	EXPECT_DOUBLE_EQ(v.sum(), 10.0);
	EXPECT_DOUBLE_EQ(v.min(), 1.0);
	EXPECT_DOUBLE_EQ(v.max(), 4.0);

	auto squared = v.apply([](double x) { return x * x; });
	EXPECT_DOUBLE_EQ(squared[2], 9.0);
}

TEST(ValarrayHeader, SliceAndMaskBasedSelections)
{
	// slice/mask_array enable sub-selection assignment and extraction.
	std::valarray<int> values{ 0, 1, 2, 3, 4, 5 };
	std::slice even(0, 3, 2);
	values[even] = std::valarray<int>{ 10, 20, 30 };
	EXPECT_EQ(values[0], 10);
	EXPECT_EQ(values[2], 20);
	EXPECT_EQ(values[4], 30);
}

}  // namespace
