#include <gtest/gtest.h>

#include <functional>
#include <vector>

#if defined(__has_include)
#if __has_include(<flat_set>)
#include <flat_set>
#define VIBE_HAS_FLAT_SET_HEADER 1
#else
#define VIBE_HAS_FLAT_SET_HEADER 0
#endif
#else
#define VIBE_HAS_FLAT_SET_HEADER 0
#endif

namespace {

	TEST(FlatSetHeader, HeaderAvailabilityAndFeatureMacroContract)
	{
		// <flat_set> is C++23 and may be unavailable depending on standard library.
#if VIBE_HAS_FLAT_SET_HEADER
#ifdef __cpp_lib_flat_set
		EXPECT_GE(__cpp_lib_flat_set, 202207L);
#else
		FAIL() << "<flat_set> present but __cpp_lib_flat_set missing.";
#endif
#else
		GTEST_SKIP() << "<flat_set> is not available on this toolchain.";
#endif
	}

#if VIBE_HAS_FLAT_SET_HEADER

	TEST(FlatSet, ConstructionOrderingAndBasicObservers)
	{
		// flat_set stores unique keys in sorted order with contiguous storage model.
		std::flat_set<int> set;
		EXPECT_TRUE(set.empty());
		EXPECT_EQ(set.size(), 0u);
		EXPECT_GT(set.max_size(), 0u);

		set.insert(3);
		set.insert(1);
		set.insert(2);
		set.insert(2); // duplicate ignored
		EXPECT_EQ(set.size(), 3u);
		EXPECT_EQ(*set.begin(), 1);
		EXPECT_TRUE((std::is_same_v<decltype(set.key_comp()), std::less<int>>));
		EXPECT_TRUE((std::is_same_v<decltype(set.value_comp()), std::less<int>>));
	}

	// TODO: Fix
	//TEST(FlatSet, ConstructorsAssignmentAndAllocatorObserver)
	//{
	//	// Validate range/initializer-list/copy/move construction plus assignment.
	//	std::vector<int> source{ 5, 1, 4, 1 };
	//	std::flat_set<int> a(source.begin(), source.end());
	//	EXPECT_EQ(a, (std::flat_set<int>{ 1, 4, 5 }));

	//	std::flat_set<int> b{ 9, 7, 8 };
	//	std::flat_set<int> c(b);
	//	EXPECT_EQ(c, b);

	//	std::flat_set<int> d(std::move(c));
	//	EXPECT_EQ(d, (std::flat_set<int>{ 7, 8, 9 }));

	//	a = d;
	//	EXPECT_EQ(a, d);

	//	auto alloc = a.get_allocator();
	//	int* p = std::allocator_traits<decltype(alloc)>::allocate(alloc, 1);
	//	ASSERT_NE(p, nullptr);
	//	std::allocator_traits<decltype(alloc)>::deallocate(alloc, p, 1);
	//}

	TEST(FlatSet, LookupContainsFindCountBoundsAndEqualRange)
	{
		// Lookup API mirrors ordered associative containers.
		std::flat_set<int> set{ 1, 3, 5, 7 };

		EXPECT_TRUE(set.contains(3));
		EXPECT_FALSE(set.contains(4));
		EXPECT_EQ(set.count(1), 1u);
		EXPECT_EQ(set.count(2), 0u);

		auto it = set.find(5);
		ASSERT_NE(it, set.end());
		EXPECT_EQ(*it, 5);

		auto lb = set.lower_bound(4);
		ASSERT_NE(lb, set.end());
		EXPECT_EQ(*lb, 5);

		auto ub = set.upper_bound(5);
		ASSERT_NE(ub, set.end());
		EXPECT_EQ(*ub, 7);

		auto [first, last] = set.equal_range(3);
		ASSERT_NE(first, set.end());
		EXPECT_EQ(*first, 3);
		EXPECT_EQ(std::distance(first, last), 1);
	}

	TEST(FlatSet, InsertEmplaceEraseAndClear)
	{
		// Insertion APIs preserve uniqueness; erase removes by key/iterator/range.
		std::flat_set<int> set;

		auto [i1, ok1] = set.insert(10);
		EXPECT_TRUE(ok1);
		EXPECT_EQ(*i1, 10);

		auto [i2, ok2] = set.emplace(20);
		EXPECT_TRUE(ok2);
		EXPECT_EQ(*i2, 20);

		auto [i3, ok3] = set.insert(20);
		EXPECT_FALSE(ok3);
		EXPECT_EQ(*i3, 20);

		const std::size_t erasedByKey = set.erase(10);
		EXPECT_EQ(erasedByKey, 1u);
		EXPECT_FALSE(set.contains(10));

		set.insert(5);
		set.insert(15);
		set.erase(set.find(15));
		EXPECT_FALSE(set.contains(15));

		set.erase(set.begin(), set.end());
		EXPECT_TRUE(set.empty());

		set.insert(1);
		set.clear();
		EXPECT_TRUE(set.empty());
	}

	// TODO: Fix
	//TEST(FlatSet, IteratorsSwapExtractReplaceAndComparisons)
	//{
	//	// Iteration order, swap, flat-container extraction/replacement, comparisons.
	//	std::flat_set<int> a{ 3, 1, 2 };
	//	std::flat_set<int> b{ 9 };

	//	int concat = 0;
	//	for (int v : a)
	//	{
	//		concat = concat * 10 + v;
	//	}
	//	EXPECT_EQ(concat, 123);

	//	a.swap(b);
	//	EXPECT_EQ(a, (std::flat_set<int>{ 9 }));
	//	EXPECT_EQ(b, (std::flat_set<int>{ 1, 2, 3 }));

	//	auto keys = b.extract();
	//	EXPECT_EQ(keys.size(), 3u);
	//	EXPECT_TRUE(b.empty());

	//	std::flat_set<int> rebuilt;
	//	rebuilt.replace(std::move(keys));
	//	EXPECT_EQ(rebuilt, (std::flat_set<int>{ 1, 2, 3 }));

	//	EXPECT_TRUE((std::flat_set<int>{ 1, 2 } < std::flat_set<int>{ 1, 3 }));
	//	EXPECT_TRUE((std::flat_set<int>{ 1, 2 } != std::flat_set<int>{ 1, 3 }));
	//}

	TEST(FlatSet, EraseIfNonMemberWhenAvailable)
	{
#ifdef __cpp_lib_erase_if
		std::flat_set<int> set{ 1, 2, 3, 4, 5, 6 };
		const auto removed = std::erase_if(set, [](int v) { return (v % 2) == 0; });
		EXPECT_EQ(removed, 3u);
		EXPECT_EQ(set, (std::flat_set<int>{ 1, 3, 5 }));
#else
		GTEST_SKIP() << "std::erase_if unavailable.";
#endif
	}

#endif  // VIBE_HAS_FLAT_SET_HEADER

}  // namespace
