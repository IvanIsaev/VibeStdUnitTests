#include <gtest/gtest.h>

#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace {

TEST(SetHeader, ConstructorsAssignmentAndAllocatorAccess)
{
	// std::set maintains unique sorted keys with common constructor families.
	std::set<int> a;
	EXPECT_TRUE(a.empty());

	std::vector<int> src{ 3, 1, 2, 2 };
	std::set<int> b(src.begin(), src.end());
	EXPECT_EQ(b, (std::set<int>{ 1, 2, 3 }));

	std::set<int> c{ 9, 7, 8 };
	std::set<int> d(c);
	EXPECT_EQ(d, c);

	std::set<int> e(std::move(d));
	EXPECT_EQ(e.size(), 3u);
	a = e;
	EXPECT_EQ(a, e);

	auto alloc = a.get_allocator();
	int* p = std::allocator_traits<decltype(alloc)>::allocate(alloc, 1);
	ASSERT_NE(p, nullptr);
	std::allocator_traits<decltype(alloc)>::deallocate(alloc, p, 1);
}

TEST(SetHeader, IteratorsCapacityAndComparatorObservers)
{
	// set iterators traverse in sorted order according to key comparator.
	std::set<int> s{ 4, 1, 3, 2 };
	EXPECT_FALSE(s.empty());
	EXPECT_EQ(s.size(), 4u);
	EXPECT_GT(s.max_size(), 0u);

	std::vector<int> observed(s.begin(), s.end());
	EXPECT_EQ(observed, (std::vector<int>{ 1, 2, 3, 4 }));

	auto comp = s.key_comp();
	EXPECT_TRUE(comp(1, 2));
	EXPECT_FALSE(comp(2, 1));
}

TEST(SetHeader, InsertEmplaceEraseAndClearFamilies)
{
	// set insertion APIs enforce uniqueness and expose success flags.
	std::set<int> s;
	auto [it1, inserted1] = s.insert(1);
	EXPECT_TRUE(inserted1);
	EXPECT_EQ(*it1, 1);

	auto [it2, inserted2] = s.emplace(2);
	EXPECT_TRUE(inserted2);
	EXPECT_EQ(*it2, 2);

	auto [it3, inserted3] = s.insert(2);
	EXPECT_FALSE(inserted3);
	EXPECT_EQ(*it3, 2);

	const auto removed = s.erase(1);
	EXPECT_EQ(removed, 1u);
	EXPECT_FALSE(s.contains(1));

	s.clear();
	EXPECT_TRUE(s.empty());
}

TEST(SetHeader, LookupAndBoundsQueries)
{
	// count/find/contains and bound queries support logarithmic lookup.
	std::set<int> s{ 1, 3, 5, 7 };
	EXPECT_EQ(s.count(3), 1u);
	EXPECT_EQ(s.count(2), 0u);
	EXPECT_TRUE(s.contains(5));
	EXPECT_FALSE(s.contains(6));

	auto lb = s.lower_bound(4);
	ASSERT_NE(lb, s.end());
	EXPECT_EQ(*lb, 5);

	auto ub = s.upper_bound(5);
	ASSERT_NE(ub, s.end());
	EXPECT_EQ(*ub, 7);

	auto [first, second] = s.equal_range(3);
	ASSERT_NE(first, s.end());
	EXPECT_EQ(*first, 3);
	EXPECT_EQ(*second, 5);
}

TEST(SetHeader, ExtractMergeSwapAndTransparentLookup)
{
	// Node-handle extract/merge enable key transfer between compatible sets.
	std::set<std::string, std::less<>> a{ "alpha", "beta" };
	std::set<std::string, std::less<>> b{ "gamma" };

	auto node = a.extract("beta");
	EXPECT_FALSE(node.empty());
	EXPECT_EQ(node.value(), "beta");
	EXPECT_FALSE(a.contains("beta"));
	b.insert(std::move(node));
	EXPECT_TRUE(b.contains("beta"));

	a.merge(b);
	EXPECT_TRUE(a.contains("gamma"));

	std::set<std::string, std::less<>> c{ "delta" };
	a.swap(c);
	EXPECT_TRUE(a.contains("delta"));

	const std::string_view key = "delta";
	auto it = a.find(key);
	ASSERT_NE(it, a.end());
	EXPECT_EQ(*it, "delta");
}

TEST(SetHeader, NonMemberEraseIfWhenAvailable)
{
	// C++20 non-member erase_if removes elements satisfying a predicate.
#ifdef __cpp_lib_erase_if
	std::set<int> values{ 1, 2, 3, 4, 5, 6 };
	const auto removed = std::erase_if(values, [](int v) { return (v % 2) == 0; });
	EXPECT_EQ(removed, 3u);
	EXPECT_EQ(values, (std::set<int>{ 1, 3, 5 }));
#endif
}

}  // namespace
