#include <gtest/gtest.h>

#include <numeric>
#include <string>
#include <vector>

namespace {

TEST(VectorHeader, ConstructorsAssignmentAndAllocatorAccess)
{
	// vector supports standard sequence constructor families and assign().
	std::vector<int> a;
	EXPECT_TRUE(a.empty());

	std::vector<int> b(3, 7);
	EXPECT_EQ(b, (std::vector<int>{ 7, 7, 7 }));

	std::vector<int> c{ 1, 2, 3 };
	std::vector<int> d(c);
	EXPECT_EQ(d, c);

	std::vector<int> e(std::move(d));
	EXPECT_EQ(e, (std::vector<int>{ 1, 2, 3 }));

	a.assign(2, 5);
	EXPECT_EQ(a, (std::vector<int>{ 5, 5 }));
	a.assign(c.begin(), c.end());
	EXPECT_EQ(a, (std::vector<int>{ 1, 2, 3 }));
	a.assign({ 4, 5 });
	EXPECT_EQ(a, (std::vector<int>{ 4, 5 }));

	auto alloc = a.get_allocator();
	int* p = std::allocator_traits<decltype(alloc)>::allocate(alloc, 1);
	ASSERT_NE(p, nullptr);
	std::allocator_traits<decltype(alloc)>::deallocate(alloc, p, 1);
}

TEST(VectorHeader, ElementAccessAndContiguousData)
{
	// vector guarantees contiguous storage and rich element access APIs.
	std::vector<std::string> v{ "a", "b", "c" };
	EXPECT_EQ(v.front(), "a");
	EXPECT_EQ(v.back(), "c");
	EXPECT_EQ(v[1], "b");
	EXPECT_EQ(v.at(2), "c");
	EXPECT_THROW((void)v.at(3), std::out_of_range);

	auto* ptr = v.data();
	ASSERT_NE(ptr, nullptr);
	EXPECT_EQ(ptr[0], "a");
}

TEST(VectorHeader, IteratorsCapacityReserveAndShrinkToFit)
{
	// begin/end iterators pair with capacity controls reserve/capacity/shrink_to_fit.
	std::vector<int> v{ 1, 2, 3, 4 };
	const int sum = std::accumulate(v.begin(), v.end(), 0);
	EXPECT_EQ(sum, 10);

	const auto oldCap = v.capacity();
	v.reserve(oldCap + 16);
	EXPECT_GE(v.capacity(), oldCap + 16);
	EXPECT_EQ(v.size(), 4u);

	v.resize(10, 9);
	EXPECT_EQ(v.size(), 10u);
	v.shrink_to_fit();
	EXPECT_GE(v.capacity(), v.size());
}

TEST(VectorHeader, ModifiersPushPopEmplaceInsertEraseClear)
{
	// vector supports all common sequence modifier families.
	std::vector<int> v;
	v.push_back(2);
	v.emplace_back(3);
	v.insert(v.begin(), 1);
	EXPECT_EQ(v, (std::vector<int>{ 1, 2, 3 }));

	v.insert(v.end(), 2, 4);
	EXPECT_EQ(v, (std::vector<int>{ 1, 2, 3, 4, 4 }));

	v.insert(v.begin(), { -1, 0 });
	EXPECT_EQ(v.front(), -1);

	v.erase(v.begin());         // erase -1
	v.erase(v.begin(), v.begin() + 1);  // erase 0
	v.pop_back();
	EXPECT_EQ(v, (std::vector<int>{ 1, 2, 3, 4 }));

	v.clear();
	EXPECT_TRUE(v.empty());
}

TEST(VectorHeader, SwapComparisonsAndNonMemberEraseHelpers)
{
	// vector compares lexicographically and supports C++20 erase helpers.
	std::vector<int> a{ 1, 2, 3 };
	std::vector<int> b{ 1, 2, 4 };
	EXPECT_TRUE(a < b);
	EXPECT_TRUE(a != b);

	a.swap(b);
	EXPECT_EQ(a, (std::vector<int>{ 1, 2, 4 }));

	using std::swap;
	swap(a, b);
	EXPECT_EQ(a, (std::vector<int>{ 1, 2, 3 }));

#ifdef __cpp_lib_erase_if
	std::vector<int> c{ 1, 2, 2, 3, 4, 5, 6 };
	const auto removedEq = std::erase(c, 2);
	EXPECT_EQ(removedEq, 2u);
	const auto removedPred = std::erase_if(c, [](int v) { return (v % 2) == 0; });
	EXPECT_EQ(removedPred, 2u);  // removed 4 and 6
	EXPECT_EQ(c, (std::vector<int>{ 1, 3, 5 }));
#endif
}

}  // namespace
