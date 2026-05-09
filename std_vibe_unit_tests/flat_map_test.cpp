#include <gtest/gtest.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

#if defined(__has_include)
#if __has_include(<flat_map>)
#include <flat_map>
#define VIBE_HAS_FLAT_MAP_HEADER 1
#else
#define VIBE_HAS_FLAT_MAP_HEADER 0
#endif
#else
#define VIBE_HAS_FLAT_MAP_HEADER 0
#endif

namespace {

	TEST(FlatMapHeader, HeaderAvailabilityAndFeatureMacroContract)
	{
		// <flat_map> is a C++23 container header and may be absent on some standard
		// library implementations. Keep suite portable while validating macro contract.
#if VIBE_HAS_FLAT_MAP_HEADER
#ifdef __cpp_lib_flat_map
		EXPECT_GE(__cpp_lib_flat_map, 202207L);
#else
		FAIL() << "<flat_map> present but __cpp_lib_flat_map missing.";
#endif
#else
		GTEST_SKIP() << "<flat_map> is not available on this toolchain.";
#endif
	}

#if VIBE_HAS_FLAT_MAP_HEADER

	TEST(FlatMap, ConstructionAndObserverBasics)
	{
		// flat_map stores sorted unique keys with contiguous storage semantics.
		std::flat_map<int, std::string> map;
		EXPECT_TRUE(map.empty());
		EXPECT_EQ(map.size(), 0u);
		EXPECT_GT(map.max_size(), 0u);

		map.insert({ 2, "two" });
		map.insert({ 1, "one" });
		EXPECT_EQ(map.size(), 2u);
		EXPECT_EQ(map.begin()->first, 1); // sorted by key

		EXPECT_TRUE((std::is_same_v<decltype(map.key_comp()), std::less<int>>));
		EXPECT_TRUE((std::is_same_v<decltype(map.value_comp()), std::flat_map<int, std::string>::value_compare>));
	}

	TEST(FlatMap, ElementAccessAndAtRangeChecking)
	{
		// operator[] inserts default value for missing key; at() is checked access.
		std::flat_map<std::string, int> map;
		map["alpha"] = 1;
		map["beta"] = 2;

		EXPECT_EQ(map["alpha"], 1);
		EXPECT_EQ(map.at("beta"), 2);
		EXPECT_THROW((void)map.at("gamma"), std::out_of_range);
	}

	TEST(FlatMap, LookupContainsFindCountBoundsAndEqualRange)
	{
		// Lookup APIs mirror associative container contracts.
		std::flat_map<int, char> map{ { 1, 'a' }, { 3, 'c' }, { 5, 'e' } };

		EXPECT_TRUE(map.contains(3));
		EXPECT_FALSE(map.contains(4));
		EXPECT_EQ(map.count(1), 1u);
		EXPECT_EQ(map.count(2), 0u);

		auto it = map.find(5);
		ASSERT_NE(it, map.end());
		EXPECT_EQ(it->second, 'e');

		auto lb = map.lower_bound(2);
		ASSERT_NE(lb, map.end());
		EXPECT_EQ(lb->first, 3);

		auto ub = map.upper_bound(3);
		ASSERT_NE(ub, map.end());
		EXPECT_EQ(ub->first, 5);

		auto [first, last] = map.equal_range(3);
		ASSERT_NE(first, map.end());
		EXPECT_EQ(first->first, 3);
		EXPECT_EQ(std::distance(first, last), 1);
	}

	TEST(FlatMap, InsertEmplaceTryEmplaceInsertOrAssignAndErase)
	{
		// Modifier APIs provide all common associative update patterns.
		std::flat_map<int, std::string> map;

		auto [i1, inserted1] = map.insert({ 1, "one" });
		EXPECT_TRUE(inserted1);
		EXPECT_EQ(i1->second, "one");

		auto [i2, inserted2] = map.emplace(2, "two");
		EXPECT_TRUE(inserted2);
		EXPECT_EQ(i2->second, "two");

		auto [i3, inserted3] = map.try_emplace(2, "override");
		EXPECT_FALSE(inserted3);
		EXPECT_EQ(i3->second, "two");

		auto [i4, inserted4] = map.insert_or_assign(2, "TWO");
		EXPECT_FALSE(inserted4);
		EXPECT_EQ(i4->second, "TWO");

		const std::size_t erased = map.erase(1);
		EXPECT_EQ(erased, 1u);
		EXPECT_FALSE(map.contains(1));
	}

	TEST(FlatMap, IteratorAccessSwapAndClear)
	{
		// Iterators expose sorted traversal; swap and clear manage container state.
		std::flat_map<int, int> a{ { 3, 30 }, { 1, 10 }, { 2, 20 } };
		std::flat_map<int, int> b{ { 9, 90 } };

		int keyConcat = 0;
		for (auto it = a.begin(); it != a.end(); ++it)
		{
			keyConcat = keyConcat * 10 + it->first;
		}
		EXPECT_EQ(keyConcat, 123);

		a.swap(b);
		EXPECT_EQ(a.size(), 1u);
		EXPECT_TRUE(a.contains(9));
		EXPECT_EQ(b.size(), 3u);

		b.clear();
		EXPECT_TRUE(b.empty());
	}

	TEST(FlatMap, ExtractAndReplaceContainersInterfaceWhenAvailable)
	{
		// C++23 flat_map exposes key/value container extraction and replacement APIs.
		std::flat_map<int, std::string> map{ { 1, "one" }, { 2, "two" } };

		auto keys = map.extract();
		EXPECT_EQ(keys.keys.size(), 2u);
		EXPECT_EQ(keys.values.size(), 2u);
		EXPECT_TRUE(map.empty());

		std::flat_map<int, std::string> rebuilt;
		rebuilt.replace(std::move(keys.keys), std::move(keys.values));
		EXPECT_EQ(rebuilt.size(), 2u);
		EXPECT_EQ(rebuilt.at(1), "one");
		EXPECT_EQ(rebuilt.at(2), "two");
	}

	TEST(FlatMap, ComparisonOperators)
	{
		// flat_map supports equality and lexicographic ordering comparisons.
		std::flat_map<int, int> a{ { 1, 1 }, { 2, 2 } };
		std::flat_map<int, int> b{ { 1, 1 }, { 2, 2 } };
		std::flat_map<int, int> c{ { 1, 1 }, { 3, 3 } };

		EXPECT_TRUE(a == b);
		EXPECT_FALSE(a != b);
		EXPECT_TRUE(a < c);
		EXPECT_TRUE(c > a);
	}

	TEST(FlatMap, EraseIfNonMemberWhenAvailable)
	{
		// std::erase_if supports predicate-based removal for flat associative containers.
#ifdef __cpp_lib_erase_if
		std::flat_map<int, int> map{ { 1, 10 }, { 2, 20 }, { 3, 30 }, { 4, 40 } };
		const auto removed = std::erase_if(map, [](const auto& kv) { return (kv.first % 2) == 0; });
		EXPECT_EQ(removed, 2u);
		EXPECT_EQ(map.size(), 2u);
		EXPECT_TRUE(map.contains(1));
		EXPECT_TRUE(map.contains(3));
#else
		GTEST_SKIP() << "std::erase_if unavailable.";
#endif
	}

#endif  // VIBE_HAS_FLAT_MAP_HEADER

}  // namespace
