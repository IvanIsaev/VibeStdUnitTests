#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace {

TEST(UnorderedSetHeader, ConstructorsAssignmentAndBucketContracts)
{
	// unordered_set stores unique keys by hash; order is unspecified.
	std::unordered_set<int> a;
	EXPECT_TRUE(a.empty());

	std::unordered_set<int> b{ 1, 2, 3, 3 };
	EXPECT_EQ(b.size(), 3u);

	std::unordered_set<int> c(b);
	EXPECT_EQ(c, b);

	std::unordered_set<int> d(std::move(c));
	EXPECT_EQ(d.size(), 3u);
	a = d;
	EXPECT_EQ(a, d);

	EXPECT_GT(a.bucket_count(), 0u);
	EXPECT_GT(a.max_load_factor(), 0.0f);
}

TEST(UnorderedSetHeader, InsertEmplaceEraseAndClearOperations)
{
	// Insert/emplace return iterator+inserted flag; duplicates are rejected.
	std::unordered_set<int> s;
	auto [it1, inserted1] = s.insert(1);
	EXPECT_TRUE(inserted1);
	EXPECT_EQ(*it1, 1);

	auto [it2, inserted2] = s.emplace(2);
	EXPECT_TRUE(inserted2);
	EXPECT_EQ(*it2, 2);

	auto [it3, inserted3] = s.insert(2);
	EXPECT_FALSE(inserted3);
	EXPECT_EQ(*it3, 2);

	const auto removedByKey = s.erase(1);
	EXPECT_EQ(removedByKey, 1u);
	EXPECT_FALSE(s.contains(1));

	s.clear();
	EXPECT_TRUE(s.empty());
}

TEST(UnorderedSetHeader, LookupAndEqualRangeQueries)
{
	// count/find/contains/equal_range provide lookup semantics for hash sets.
	std::unordered_set<int> s{ 1, 3, 5 };
	EXPECT_EQ(s.count(3), 1u);
	EXPECT_EQ(s.count(2), 0u);
	EXPECT_TRUE(s.contains(5));
	EXPECT_FALSE(s.contains(4));

	auto it = s.find(1);
	ASSERT_NE(it, s.end());
	EXPECT_EQ(*it, 1);

	auto [first, second] = s.equal_range(3);
	ASSERT_NE(first, s.end());
	EXPECT_EQ(*first, 3);
	EXPECT_NE(first, second);
}

TEST(UnorderedSetHeader, ReserveRehashExtractAndMerge)
{
	// reserve/rehash tune buckets while extract/merge transfer node ownership.
	std::unordered_set<int> a{ 1, 2, 3 };
	const auto oldBucketCount = a.bucket_count();
	a.reserve(64);
	EXPECT_GE(a.bucket_count(), oldBucketCount);
	a.rehash(128);
	EXPECT_GE(a.bucket_count(), 128u);

	auto node = a.extract(2);
	EXPECT_FALSE(node.empty());
	EXPECT_EQ(node.value(), 2);
	EXPECT_FALSE(a.contains(2));

	std::unordered_set<int> b{ 7 };
	b.insert(std::move(node));
	EXPECT_TRUE(b.contains(2));

	a.merge(b);
	EXPECT_TRUE(a.contains(7));
}

TEST(UnorderedSetHeader, TransparentLookupAndNonMemberEraseIfWhenAvailable)
{
	// Transparent hashing/equality allows heterogenous key search.
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

	std::unordered_set<std::string, TransparentHash, TransparentEq> words{ "alpha", "beta" };
	const std::string_view key = "beta";
	auto it = words.find(key);
	ASSERT_NE(it, words.end());
	EXPECT_EQ(*it, "beta");

#ifdef __cpp_lib_erase_if
	std::unordered_set<int> values{ 1, 2, 3, 4, 5, 6 };
	const auto removed = std::erase_if(values, [](int v) { return (v % 2) == 0; });
	EXPECT_EQ(removed, 3u);
#endif
}

}  // namespace
