#include <gtest/gtest.h>

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

TEST(MapHeader, ConstructorsAssignmentAndAllocatorBasics)
{
	// std::map supports default/range/copy/move/init-list constructors and assign.
	std::map<int, std::string> a;
	EXPECT_TRUE(a.empty());

	std::vector<std::pair<int, std::string>> src{ { 2, "b" }, { 1, "a" } };
	std::map<int, std::string> b(src.begin(), src.end());
	EXPECT_EQ(b.size(), 2u);
	EXPECT_EQ(b.begin()->first, 1);  // key order is sorted ascending by default.

	std::map<int, std::string> c{ { 3, "c" }, { 4, "d" } };
	std::map<int, std::string> d(c);
	EXPECT_EQ(d, c);

	std::map<int, std::string> e(std::move(d));
	EXPECT_EQ(e.size(), 2u);

	a = e;
	EXPECT_EQ(a, e);

	auto alloc = a.get_allocator();
	auto* p = std::allocator_traits<decltype(alloc)>::allocate(alloc, 1);
	ASSERT_NE(p, nullptr);
	std::allocator_traits<decltype(alloc)>::deallocate(alloc, p, 1);
}

TEST(MapHeader, ElementAccessWithSubscriptAtAndInsertionByAccess)
{
	// operator[] inserts a default-mapped value if key is absent, while at()
	// performs checked access and throws for missing keys.
	std::map<int, std::string> m;
	m[1] = "one";
	EXPECT_EQ(m[1], "one");
	EXPECT_EQ(m.at(1), "one");
	EXPECT_THROW((void)m.at(42), std::out_of_range);

	// Reading via operator[] for a missing key materializes an empty mapped value.
	EXPECT_TRUE(m[2].empty());
	EXPECT_EQ(m.size(), 2u);
}

TEST(MapHeader, IteratorsCapacityAndObservers)
{
	// Iterators traverse sorted keys; observers expose comparator behavior.
	std::map<int, char> m{ { 3, 'c' }, { 1, 'a' }, { 2, 'b' } };
	EXPECT_FALSE(m.empty());
	EXPECT_EQ(m.size(), 3u);
	EXPECT_GT(m.max_size(), 0u);

	std::vector<int> keys;
	for (auto it = m.begin(); it != m.end(); ++it)
	{
		keys.push_back(it->first);
	}
	EXPECT_EQ(keys, (std::vector<int>{ 1, 2, 3 }));

	auto comp = m.key_comp();
	EXPECT_TRUE(comp(1, 2));
	EXPECT_FALSE(comp(2, 1));
}

TEST(MapHeader, InsertEmplaceTryEmplaceAndInsertOrAssign)
{
	// map offers insertion APIs for avoiding redundant construction and updating
	// existing keys with clear intent.
	std::map<int, std::string> m;

	auto [it1, inserted1] = m.insert({ 1, "one" });
	EXPECT_TRUE(inserted1);
	EXPECT_EQ(it1->second, "one");

	auto [it2, inserted2] = m.emplace(2, "two");
	EXPECT_TRUE(inserted2);
	EXPECT_EQ(it2->second, "two");

	auto [it3, inserted3] = m.try_emplace(2, "two-new");
	EXPECT_FALSE(inserted3);
	EXPECT_EQ(it3->second, "two");

	auto [it4, inserted4] = m.insert_or_assign(2, "two-assigned");
	EXPECT_FALSE(inserted4);
	EXPECT_EQ(it4->second, "two-assigned");
}

TEST(MapHeader, EraseClearSwapAndExtractNodeHandling)
{
	// Modifier APIs include erase variants, clear/swap, and node-handle extract.
	std::map<int, std::string> a{ { 1, "a" }, { 2, "b" }, { 3, "c" } };
	const auto erasedByKey = a.erase(2);
	EXPECT_EQ(erasedByKey, 1u);

	auto it = a.find(1);
	ASSERT_NE(it, a.end());
	a.erase(it);
	EXPECT_EQ(a.size(), 1u);

	std::map<int, std::string> b{ { 10, "x" } };
	a.swap(b);
	EXPECT_TRUE(a.contains(10));
	EXPECT_TRUE(b.contains(3));

	auto node = b.extract(3);
	EXPECT_FALSE(node.empty());
	EXPECT_EQ(node.key(), 3);
	EXPECT_EQ(node.mapped(), "c");
	EXPECT_FALSE(b.contains(3));

	b.clear();
	EXPECT_TRUE(b.empty());
}

TEST(MapHeader, LookupFunctionsAndBoundsQueries)
{
	// map exposes count/find/contains and lower_bound/upper_bound/equal_range.
	std::map<int, char> m{ { 1, 'a' }, { 3, 'c' }, { 5, 'e' } };
	EXPECT_EQ(m.count(3), 1u);
	EXPECT_EQ(m.count(2), 0u);
	EXPECT_TRUE(m.contains(5));
	EXPECT_FALSE(m.contains(6));

	auto lb = m.lower_bound(2);
	ASSERT_NE(lb, m.end());
	EXPECT_EQ(lb->first, 3);

	auto ub = m.upper_bound(3);
	ASSERT_NE(ub, m.end());
	EXPECT_EQ(ub->first, 5);

	auto [first, second] = m.equal_range(3);
	ASSERT_NE(first, m.end());
	EXPECT_EQ(first->first, 3);
	EXPECT_EQ(second->first, 5);
}

TEST(MapHeader, NonMemberEraseIfAndTransparentLookupWhenAvailable)
{
	// C++20 adds erase_if for associative containers; transparent lookup allows
	// heterogeneous key search when comparator is transparent.
#ifdef __cpp_lib_erase_if
	std::map<int, int> numbers{ { 1, 1 }, { 2, 4 }, { 3, 9 }, { 4, 16 } };
	const auto removed = std::erase_if(numbers, [](const auto& kv) { return kv.first % 2 == 0; });
	EXPECT_EQ(removed, 2u);
	EXPECT_TRUE(numbers.contains(1));
	EXPECT_TRUE(numbers.contains(3));
#endif

	std::map<std::string, int, std::less<>> words{ { "alpha", 1 }, { "beta", 2 } };
	const std::string_view key = "alpha";
	auto it = words.find(key);
	ASSERT_NE(it, words.end());
	EXPECT_EQ(it->second, 1);
}

}  // namespace
#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

namespace {

	TEST(MapHeader, ConstructionElementAccessAndObservers)
	{
		std::map<int, std::string> m;
		EXPECT_TRUE(m.empty());

		m[2] = "two";
		m[1] = "one";
		EXPECT_EQ(m.size(), 2u);
		EXPECT_EQ(m.begin()->first, 1);
		EXPECT_EQ(m.at(2), "two");
		EXPECT_THROW((void)m.at(3), std::out_of_range);

		EXPECT_TRUE((std::is_same_v<decltype(m.key_comp()), std::less<int>>));
	}

	TEST(MapHeader, LookupAndModifierAPI)
	{
		std::map<int, std::string> m{ { 1, "a" }, { 3, "c" } };
		auto [it, inserted] = m.insert({ 2, "b" });
		EXPECT_TRUE(inserted);
		EXPECT_EQ(it->second, "b");

		auto [it2, inserted2] = m.try_emplace(3, "new");
		EXPECT_FALSE(inserted2);
		EXPECT_EQ(it2->second, "c");

		auto [it3, inserted3] = m.insert_or_assign(3, "C");
		EXPECT_FALSE(inserted3);
		EXPECT_EQ(it3->second, "C");

		EXPECT_TRUE(m.contains(2));
		EXPECT_EQ(m.count(9), 0u);
		EXPECT_NE(m.find(1), m.end());

		EXPECT_EQ(m.erase(2), 1u);
		EXPECT_FALSE(m.contains(2));
	}

	TEST(MapHeader, BoundsEqualRangeExtractMergeAndEraseIf)
	{
		std::map<int, int> a{ { 1, 10 }, { 3, 30 }, { 5, 50 } };
		auto lb = a.lower_bound(2);
		ASSERT_NE(lb, a.end());
		EXPECT_EQ(lb->first, 3);
		auto ub = a.upper_bound(3);
		ASSERT_NE(ub, a.end());
		EXPECT_EQ(ub->first, 5);

		auto [first, last] = a.equal_range(3);
		EXPECT_EQ(std::distance(first, last), 1);

		std::map<int, int> b{ { 7, 70 } };
		b.merge(a);
		EXPECT_TRUE(b.contains(1));
		EXPECT_TRUE(a.empty());

		auto nh = b.extract(7);
		EXPECT_TRUE(!nh.empty());
		EXPECT_FALSE(b.contains(7));
		b.insert(std::move(nh));
		EXPECT_TRUE(b.contains(7));

#ifdef __cpp_lib_erase_if
		EXPECT_EQ(std::erase_if(b, [](const auto& kv) { return kv.first % 2 == 1; }), b.size());
		EXPECT_TRUE(b.empty());
#endif
	}

}  // namespace
