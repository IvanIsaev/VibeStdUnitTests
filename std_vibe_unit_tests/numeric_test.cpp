#include <gtest/gtest.h>

#include <array>
#include <numeric>
#include <vector>

namespace {

TEST(NumericHeader, AccumulateInnerProductAndReduce)
{
	// Basic numeric algorithms aggregate sequences with optional custom ops.
	std::vector<int> a{ 1, 2, 3, 4 };
	std::vector<int> b{ 5, 6, 7, 8 };
	EXPECT_EQ(std::accumulate(a.begin(), a.end(), 0), 10);
	EXPECT_EQ(std::inner_product(a.begin(), a.end(), b.begin(), 0), 70);
	EXPECT_EQ(std::reduce(a.begin(), a.end(), 0), 10);
}

TEST(NumericHeader, PartialSumAdjacentDifferenceAndInclusiveExclusiveScan)
{
	// Prefix and difference algorithms are core building blocks for transforms.
	std::vector<int> src{ 1, 2, 3, 4 };
	std::vector<int> out(4);

	std::partial_sum(src.begin(), src.end(), out.begin());
	EXPECT_EQ(out, (std::vector<int>{ 1, 3, 6, 10 }));

	std::adjacent_difference(src.begin(), src.end(), out.begin());
	EXPECT_EQ(out, (std::vector<int>{ 1, 1, 1, 1 }));

	std::inclusive_scan(src.begin(), src.end(), out.begin());
	EXPECT_EQ(out, (std::vector<int>{ 1, 3, 6, 10 }));

	std::exclusive_scan(src.begin(), src.end(), out.begin(), 0);
	EXPECT_EQ(out, (std::vector<int>{ 0, 1, 3, 6 }));
}

TEST(NumericHeader, TransformReduceAndTransformScans)
{
	// Transform variants fuse mapping and accumulation/scan in one pass.
	std::array<int, 4> src{ 1, 2, 3, 4 };
	const int squaresSum = std::transform_reduce(src.begin(), src.end(), 0, std::plus<>{}, [](int v) { return v * v; });
	EXPECT_EQ(squaresSum, 30);

	std::array<int, 4> out{};
	std::transform_inclusive_scan(src.begin(), src.end(), out.begin(), std::plus<>{}, [](int v) { return v * 2; });
	EXPECT_EQ(out, (std::array<int, 4>{ 2, 6, 12, 20 }));

	std::transform_exclusive_scan(src.begin(), src.end(), out.begin(), 1, std::plus<>{}, [](int v) { return v + 1; });
	EXPECT_EQ(out, (std::array<int, 4>{ 1, 3, 6, 10 }));
}

TEST(NumericHeader, IotaGeneratesArithmeticSequence)
{
	// iota fills a range with sequentially increasing values.
	std::vector<int> values(5);
	std::iota(values.begin(), values.end(), 10);
	EXPECT_EQ(values, (std::vector<int>{ 10, 11, 12, 13, 14 }));
}

TEST(NumericHeader, GcdLcmAndMidpointHelpers)
{
	// C++17 numeric helpers include gcd/lcm and midpoint utilities.
	EXPECT_EQ(std::gcd(36, 24), 12);
	EXPECT_EQ(std::lcm(6, 8), 24);
	EXPECT_EQ(std::midpoint(10, 20), 15);
}

}  // namespace
