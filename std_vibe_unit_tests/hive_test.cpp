#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<hive>)
#include <hive>
#define VIBE_HAS_HIVE 1
#else
#define VIBE_HAS_HIVE 0
#endif

#include <numeric>
#include <vector>

namespace {

	TEST(HiveHeader, HeaderAvailabilityAndFeatureMacro)
	{
		// This test documents whether <hive> support is currently available.
#if VIBE_HAS_HIVE
#ifdef __cpp_lib_hive
		EXPECT_GE(__cpp_lib_hive, 202311L);
#endif
#else
		GTEST_SKIP() << "<hive> is not available in this standard library.";
#endif
	}

#if VIBE_HAS_HIVE

	TEST(HiveHeader, ConstructionAssignmentAndBasicStateQueries)
	{
		// std::hive provides familiar sequence-container constructor and assign forms.
		std::hive<int> a;
		EXPECT_TRUE(a.empty());
		EXPECT_EQ(a.size(), 0u);
		EXPECT_GT(a.max_size(), 0u);

		std::hive<int> b(3, 7);
		EXPECT_EQ(b.size(), 3u);

		std::hive<int> c{ 1, 2, 3 };
		EXPECT_EQ(c.size(), 3u);

		a.assign(2, 9);
		EXPECT_EQ(a.size(), 2u);
		a.assign({ 4, 5, 6 });
		EXPECT_EQ(a.size(), 3u);
	}

	TEST(HiveHeader, IterationAndCoreModifiers)
	{
		// hive supports iteration and core modifier operations like insert/erase/clear.
		std::hive<int> h{ 1, 2, 4 };
		auto inserted = h.insert(3);
		EXPECT_EQ(*inserted, 3);

		const int sum = std::accumulate(h.begin(), h.end(), 0);
		EXPECT_EQ(sum, 10);

		auto it = h.begin();
		++it;  // erase second element
		h.erase(it);
		EXPECT_EQ(h.size(), 3u);

		h.clear();
		EXPECT_TRUE(h.empty());
	}

	TEST(HiveHeader, CapacityManagementAndSwap)
	{
		// hive exposes reserve/capacity/trim_capacity and supports efficient swap.
		std::hive<int> a{ 1, 2, 3 };
		a.reserve(32);
		EXPECT_GE(a.capacity(), 32u);
		a.trim_capacity();
		EXPECT_GE(a.capacity(), a.size());

		std::hive<int> b{ 9 };
		a.swap(b);
		EXPECT_EQ(a.size(), 1u);
		EXPECT_EQ(*a.begin(), 9);
	}

}

TEST(HiveHeader, ConstructorsAssignmentAndAllocatorAccess)
{
	// std::hive is a node-based sequence container with stable references across
	// insert/erase operations (except for erased elements).
	std::hive<int> a;
	EXPECT_TRUE(a.empty());

	std::hive<int> b(3, 7);
	EXPECT_EQ(b.size(), 3u);

	std::vector<int> src{ 1, 2, 3 };
	std::hive<int> c(src.begin(), src.end());
	EXPECT_EQ(c.size(), 3u);

	std::hive<int> d{ 9, 8, 7 };
	std::hive<int> e(d);
	EXPECT_EQ(e.size(), d.size());

	std::hive<int> f(std::move(e));
	EXPECT_EQ(f.size(), 3u);

	a.assign(2, 5);
	EXPECT_EQ(a.size(), 2u);
	a.assign(src.begin(), src.end());
	EXPECT_EQ(a.size(), 3u);
	a.assign({ 4, 5 });
	EXPECT_EQ(a.size(), 2u);

	auto alloc = a.get_allocator();
	int* p = std::allocator_traits<decltype(alloc)>::allocate(alloc, 1);
	ASSERT_NE(p, nullptr);
	std::allocator_traits<decltype(alloc)>::deallocate(alloc, p, 1);
}

TEST(HiveHeader, IteratorsAndElementTraversal)
{
	// hive iterators are bidirectional and skip erased groups internally.
	std::hive<int> h{ 1, 2, 3, 4 };
	EXPECT_EQ(*h.begin(), 1);
	EXPECT_EQ(*h.rbegin(), 4);

	const int sum = std::accumulate(h.begin(), h.end(), 0);
	EXPECT_EQ(sum, 10);
}

TEST(HiveHeader, CapacityReserveAndTrimStyleOperations)
{
	// hive supports capacity-oriented operations distinct from contiguous vectors.
	std::hive<int> h;
	EXPECT_TRUE(h.empty());
	EXPECT_EQ(h.size(), 0u);
	EXPECT_GT(h.max_size(), 0u);

	h.reserve(64);
	EXPECT_GE(h.capacity(), 64u);

	h.assign({ 1, 2, 3, 4 });
	EXPECT_EQ(h.size(), 4u);
	h.trim_capacity();
	EXPECT_GE(h.capacity(), h.size());
}

TEST(HiveHeader, InsertEmplaceEraseAndReuseSemantics)
{
	// insert/emplace/erase are core hive modifiers; erased slots may be reused by
	// subsequent insertions, preserving reference stability of survivors.
	std::hive<int> h{ 1, 2, 4 };
	auto inserted = h.insert(std::next(h.begin(), 2), 3);
	EXPECT_EQ(*inserted, 3);

	auto emplaced = h.emplace(h.end(), 5);
	EXPECT_EQ(*emplaced, 5);
	EXPECT_EQ(h.size(), 5u);

	auto second = std::next(h.begin());
	h.erase(second);  // erase value 2
	EXPECT_EQ(h.size(), 4u);
	EXPECT_EQ(std::accumulate(h.begin(), h.end(), 0), 13);  // 1 + 3 + 4 + 5
}

TEST(HiveHeader, SwapAndComparisonOperations)
{
	// hive supports swap and comparisons similarly to other standard sequences.
	std::hive<int> a{ 1, 2, 3 };
	std::hive<int> b{ 1, 2, 4 };
	EXPECT_TRUE(a < b);
	EXPECT_TRUE(a != b);

	a.swap(b);
	EXPECT_EQ(std::accumulate(a.begin(), a.end(), 0), 7);

	using std::swap;
	swap(a, b);
	EXPECT_EQ(std::accumulate(a.begin(), a.end(), 0), 6);
}

TEST(HiveHeader, ConstructionIterationAndLookupLikeOperations)
{
	// std::hive is a node-based sequence container with stable references and
	// iterator invalidation properties different from vector/deque.
	std::hive<int> h{ 3, 1, 4 };
	EXPECT_EQ(h.size(), 3u);
	EXPECT_FALSE(h.empty());

	int sum = 0;
	for (int v : h) sum += v;
	EXPECT_EQ(sum, 8);

	h.push_back(10);
	h.push_front(2);
	EXPECT_EQ(h.front(), 2);
	EXPECT_EQ(h.back(), 10);
}

TEST(HiveHeader, ModifiersEraseClearAndSwap)
{
	std::hive<std::string> a{ "a", "b", "c" };
	auto it = a.begin();
	++it;
	a.erase(it);
	EXPECT_EQ(a.size(), 2u);

	std::hive<std::string> b{ "x" };
	a.swap(b);
	EXPECT_EQ(a.size(), 1u);
	EXPECT_EQ(*a.begin(), "x");

	b.clear();
	EXPECT_TRUE(b.empty());
}
#endif

}  // namespace
