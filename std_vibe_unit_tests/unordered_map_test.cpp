#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <unordered_map>

namespace {

TEST(UnorderedMapHeader, ConstructorsAssignmentAndBucketInterface)
{
	// unordered_map stores unique keys in hash buckets with average constant-time
	// lookup and insertion.
	std::unordered_map<int, std::string> a;
	EXPECT_TRUE(a.empty());

	std::unordered_map<int, std::string> b{ { 1, "one" }, { 2, "two" } };
	std::unordered_map<int, std::string> c(b);
	EXPECT_EQ(c, b);

	std::unordered_map<int, std::string> d(std::move(c));
	EXPECT_EQ(d.size(), 2u);
	a = d;
	EXPECT_EQ(a.size(), 2u);

	EXPECT_GT(a.bucket_count(), 0u);
	EXPECT_LT(a.load_factor(), a.max_load_factor() + 1.0f);
}

TEST(UnorderedMapHeader, ElementAccessAndInsertionFamilies)
{
	// operator[] inserts missing keys; at() does checked access.
	std::unordered_map<int, std::string> m;
	m[1] = "one";
	EXPECT_EQ(m.at(1), "one");
	EXPECT_THROW((void)m.at(42), std::out_of_range);
	EXPECT_TRUE(m[2].empty());

	auto [it1, inserted1] = m.insert({ 3, "three" });
	EXPECT_TRUE(inserted1);
	EXPECT_EQ(it1->second, "three");

	auto [it2, inserted2] = m.emplace(4, "four");
	EXPECT_TRUE(inserted2);
	EXPECT_EQ(it2->second, "four");

	auto [it3, inserted3] = m.try_emplace(4, "four-new");
	EXPECT_FALSE(inserted3);
	EXPECT_EQ(it3->second, "four");

	auto [it4, inserted4] = m.insert_or_assign(4, "four-assigned");
	EXPECT_FALSE(inserted4);
	EXPECT_EQ(it4->second, "four-assigned");
}

TEST(UnorderedMapHeader, FindContainsCountEraseAndClear)
{
	// Lookup and erase APIs are key operations for hash containers.
	std::unordered_map<int, int> m{ { 1, 10 }, { 2, 20 }, { 3, 30 } };
	EXPECT_EQ(m.count(2), 1u);
	EXPECT_TRUE(m.contains(3));
	EXPECT_FALSE(m.contains(4));

	auto it = m.find(1);
	ASSERT_NE(it, m.end());
	EXPECT_EQ(it->second, 10);

	const auto removedByKey = m.erase(2);
	EXPECT_EQ(removedByKey, 1u);

	it = m.find(3);
	ASSERT_NE(it, m.end());
	m.erase(it);
	EXPECT_FALSE(m.contains(3));

	m.clear();
	EXPECT_TRUE(m.empty());
}

TEST(UnorderedMapHeader, RehashReserveAndMergeExtractOperations)
{
	// reserve/rehash control bucket allocation; extract/merge move nodes.
	std::unordered_map<int, std::string> a{ { 1, "a" }, { 2, "b" } };
	const auto oldBucketCount = a.bucket_count();
	a.reserve(64);
	EXPECT_GE(a.bucket_count(), oldBucketCount);
	a.rehash(128);
	EXPECT_GE(a.bucket_count(), 128u);

	auto node = a.extract(2);
	EXPECT_FALSE(node.empty());
	EXPECT_EQ(node.key(), 2);
	EXPECT_EQ(node.mapped(), "b");

	std::unordered_map<int, std::string> b{ { 3, "c" } };
	b.insert(std::move(node));
	EXPECT_TRUE(b.contains(2));

	a.merge(b);
	EXPECT_TRUE(a.contains(3));
}

TEST(UnorderedMapHeader, TransparentLookupAndNonMemberEraseIfWhenAvailable)
{
	// With transparent hashing/equality, heterogeneous lookup can avoid temporary
	// key construction.
	struct TransparentHash
	{
		using is_transparent = void;
		std::size_t operator()(std::string_view s) const noexcept { return std::hash<std::string_view>{}(s); }
	};
	struct TransparentEq
	{
		using is_transparent = void;
		bool operator()(std::string_view a, std::string_view b) const noexcept { return a == b; }
	};

	std::unordered_map<std::string, int, TransparentHash, TransparentEq> words{ { "alpha", 1 }, { "beta", 2 } };
	const std::string_view key = "alpha";
	auto it = words.find(key);
	ASSERT_NE(it, words.end());
	EXPECT_EQ(it->second, 1);

#ifdef __cpp_lib_erase_if
	std::unordered_map<int, int> numbers{ { 1, 1 }, { 2, 4 }, { 3, 9 }, { 4, 16 } };
	const auto removed = std::erase_if(numbers, [](const auto& kv) { return kv.first % 2 == 0; });
	EXPECT_EQ(removed, 2u);
#endif
}

}  // namespace
