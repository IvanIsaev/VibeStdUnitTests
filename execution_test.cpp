#include <gtest/gtest.h>

#include <algorithm>
#include <execution>
#include <numeric>
#include <type_traits>
#include <vector>

namespace {

	TEST(Execution, PolicyObjectTypes)
	{
		// <execution> exposes four global policy objects (`seq`, `par`, `par_unseq`,
		// and `unseq`) that select how eligible standard algorithms are executed.
		// This test verifies that each object has the expected concrete policy type,
		// ensuring the header wiring and object declarations are correct.
		EXPECT_TRUE((std::is_same_v<decltype(std::execution::seq), const std::execution::sequenced_policy>));
		EXPECT_TRUE((std::is_same_v<decltype(std::execution::par), const std::execution::parallel_policy>));
		EXPECT_TRUE((std::is_same_v<decltype(std::execution::par_unseq), const std::execution::parallel_unsequenced_policy>));
		EXPECT_TRUE((std::is_same_v<decltype(std::execution::unseq), const std::execution::unsequenced_policy>));
	}

	TEST(Execution, IsExecutionPolicyTrait)
	{
		// The is_execution_policy type trait identifies whether a type is a valid
		// execution policy. This test verifies positive cases for all standard
		// policies and a negative case for a non-policy type, so template code can
		// safely branch on policy support.
		EXPECT_TRUE((std::is_execution_policy_v<std::execution::sequenced_policy>));
		EXPECT_TRUE((std::is_execution_policy_v<std::execution::parallel_policy>));
		EXPECT_TRUE((std::is_execution_policy_v<std::execution::parallel_unsequenced_policy>));
		EXPECT_TRUE((std::is_execution_policy_v<std::execution::unsequenced_policy>));
		EXPECT_FALSE((std::is_execution_policy_v<int>));
	}

	TEST(Execution, SeqForEachMutatesInOrderSemantics)
	{
		// `std::execution::seq` requests sequenced execution for policy-enabled
		// algorithms. For deterministic data transformation tasks, this ensures
		// ordinary single-thread-style semantics while still exercising the policy
		// overload. The test squares each element and validates exact output.
		std::vector<int> values{ 1, 2, 3, 4 };
		std::for_each(std::execution::seq, values.begin(), values.end(),
			[](int& x) { x = x * x; });
		EXPECT_EQ(values, (std::vector<int>{1, 4, 9, 16}));
	}

	TEST(Execution, ParReduceComputesCorrectSum)
	{
		// `std::execution::par` allows work partitioning across threads where safe.
		// This test uses reduce on a simple integer range and verifies the numerical
		// result. We validate correctness only (not scheduling), which keeps the test
		// robust across platforms and standard library implementations.
		std::vector<int> values(1000);
		std::iota(values.begin(), values.end(), 1);
		const int sum = std::reduce(std::execution::par, values.begin(), values.end(), 0);
		EXPECT_EQ(sum, 1000 * 1001 / 2);
	}

	TEST(Execution, ParUnseqTransformAppliesUnaryOperation)
	{
		// `std::execution::par_unseq` permits both parallelism and vectorization for
		// suitable operations. This test applies a pure unary transform into a
		// separate output buffer and checks element-wise correctness, demonstrating a
		// canonical safe use case for par_unseq.
		const std::vector<int> input{ 0, 1, 2, 3, 4, 5 };
		std::vector<int> output(input.size(), 0);
		std::transform(std::execution::par_unseq, input.begin(), input.end(), output.begin(),
			[](int x) { return x + 10; });
		EXPECT_EQ(output, (std::vector<int>{10, 11, 12, 13, 14, 15}));
	}

	TEST(Execution, UnseqFillWritesAllElements)
	{
		// `std::execution::unseq` allows unsequenced (vectorized) execution on a
		// single thread. This test checks that fill under unseq writes the requested
		// value to every position in the destination range.
		std::vector<int> values(32, 0);
		std::fill(std::execution::unseq, values.begin(), values.end(), 7);
		EXPECT_TRUE(std::all_of(values.begin(), values.end(), [](int x) { return x == 7; }));
	}

	TEST(Execution, AnyPolicyOverloadFindsValue)
	{
		// Many algorithms have execution-policy overloads with behavior equivalent to
		// the non-policy version from a result perspective. This test uses find with
		// each standard policy object and verifies that the target value is located,
		// proving overload viability and consistent observable result.
		const std::vector<int> values{ 3, 6, 9, 12, 15 };
		auto itSeq = std::find(std::execution::seq, values.begin(), values.end(), 12);
		auto itPar = std::find(std::execution::par, values.begin(), values.end(), 12);
		auto itParUnseq = std::find(std::execution::par_unseq, values.begin(), values.end(), 12);
		auto itUnseq = std::find(std::execution::unseq, values.begin(), values.end(), 12);

		ASSERT_NE(itSeq, values.end());
		ASSERT_NE(itPar, values.end());
		ASSERT_NE(itParUnseq, values.end());
		ASSERT_NE(itUnseq, values.end());
		EXPECT_EQ(*itSeq, 12);
		EXPECT_EQ(*itPar, 12);
		EXPECT_EQ(*itParUnseq, 12);
		EXPECT_EQ(*itUnseq, 12);
	}

	TEST(Execution, StableSortWithSeqMaintainsStableOrder)
	{
		// Some policy-enabled algorithms (such as stable_sort with sequenced policy)
		// guarantee extra ordering properties beyond basic sortedness. Here we sort
		// by the first field and verify that records with equal keys preserve their
		// original relative order, confirming stability semantics.
		using Pair = std::pair<int, char>;
		std::vector<Pair> values{
			{2, 'a'},
			{1, 'x'},
			{2, 'b'},
			{1, 'y'}
		};

		std::stable_sort(std::execution::seq, values.begin(), values.end(),
			[](const Pair& lhs, const Pair& rhs) { return lhs.first < rhs.first; });

		EXPECT_EQ(values[0], (Pair{1, 'x'}));
		EXPECT_EQ(values[1], (Pair{1, 'y'}));
		EXPECT_EQ(values[2], (Pair{2, 'a'}));
		EXPECT_EQ(values[3], (Pair{2, 'b'}));
	}

}  // namespace
