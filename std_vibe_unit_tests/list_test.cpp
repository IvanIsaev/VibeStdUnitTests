#include <gtest/gtest.h>

#include <algorithm>
#include <list>
#include <numeric>
#include <string>
#include <vector>

namespace {

TEST(ListHeader, ConstructorsAssignmentAndAllocatorAccess)
{
	// std::list supports default, count/value, range, copy, move, and
	// initializer-list construction along with assign() families.
	std::list<int> a;
	EXPECT_TRUE(a.empty());

	std::list<int> b(3, 7);
	EXPECT_EQ(b, (std::list<int>{ 7, 7, 7 }));

	std::vector<int> src{ 1, 2, 3 };
	std::list<int> c(src.begin(), src.end());
	EXPECT_EQ(c, (std::list<int>{ 1, 2, 3 }));

	std::list<int> d{ 9, 8, 7 };
	std::list<int> e(d);
	EXPECT_EQ(e, d);

	std::list<int> f(std::move(e));
	EXPECT_EQ(f, (std::list<int>{ 9, 8, 7 }));

	a.assign(2, 5);
	EXPECT_EQ(a, (std::list<int>{ 5, 5 }));
	a.assign(src.begin(), src.end());
	EXPECT_EQ(a, (std::list<int>{ 1, 2, 3 }));
	a.assign({ 4, 5 });
	EXPECT_EQ(a, (std::list<int>{ 4, 5 }));

	auto alloc = a.get_allocator();
	int* p = std::allocator_traits<decltype(alloc)>::allocate(alloc, 1);
	ASSERT_NE(p, nullptr);
	std::allocator_traits<decltype(alloc)>::deallocate(alloc, p, 1);
}

TEST(ListHeader, FrontBackAndIteratorFamilyTraversal)
{
	// std::list provides bidirectional iterators and front/back element access.
	std::list<int> values{ 1, 2, 3, 4 };
	EXPECT_EQ(values.front(), 1);
	EXPECT_EQ(values.back(), 4);

	const int sum = std::accumulate(values.begin(), values.end(), 0);
	EXPECT_EQ(sum, 10);

	const std::list<int>& cvalues = values;
	EXPECT_EQ(*cvalues.cbegin(), 1);
	EXPECT_EQ(*cvalues.crbegin(), 4);
}

TEST(ListHeader, CapacityAndResizeSemantics)
{
	// list exposes empty/size/max_size and resize APIs.
	std::list<int> values;
	EXPECT_TRUE(values.empty());
	EXPECT_EQ(values.size(), 0u);
	EXPECT_GT(values.max_size(), 0u);

	values.resize(3);
	EXPECT_EQ(values, (std::list<int>{ 0, 0, 0 }));
	values.resize(5, 9);
	EXPECT_EQ(values, (std::list<int>{ 0, 0, 0, 9, 9 }));
	values.resize(2);
	EXPECT_EQ(values, (std::list<int>{ 0, 0 }));
}

TEST(ListHeader, PushPopEmplaceInsertAndEraseFamilies)
{
	// list supports front/back insertion plus middle insert/emplace/erase forms.
	std::list<int> values;
	values.push_back(2);
	values.push_front(1);
	values.emplace_back(4);
	values.emplace_front(0);
	EXPECT_EQ(values, (std::list<int>{ 0, 1, 2, 4 }));

	auto it = std::next(values.begin(), 3);
	values.emplace(it, 3);
	EXPECT_EQ(values, (std::list<int>{ 0, 1, 2, 3, 4 }));

	values.insert(values.end(), 2, 5);
	EXPECT_EQ(values, (std::list<int>{ 0, 1, 2, 3, 4, 5, 5 }));
	values.insert(values.begin(), { -1, -2 });

	values.erase(values.begin());  // erase -1
	values.pop_front();            // erase -2
	values.pop_back();             // erase trailing 5
	EXPECT_EQ(values, (std::list<int>{ 0, 1, 2, 3, 4, 5 }));
}

TEST(ListHeader, RemoveUniqueReverseAndSort)
{
	// list has linked-list specialized algorithms for value/predicate removal,
	// duplicate squashing, reversal, and in-place sort.
	std::list<int> values{ 4, 2, 2, 1, 3, 3, 5 };
	values.remove(2);
	values.remove_if([](int v) { return v == 5; });
	values.unique();
	values.sort();
	values.reverse();
	EXPECT_EQ(values, (std::list<int>{ 4, 3, 1 }));
}

TEST(ListHeader, SpliceAndMergeTransferNodesWithoutCopies)
{
	// splice/merge move nodes between lists and preserve list ordering contracts.
	std::list<int> a{ 1, 3, 5 };
	std::list<int> b{ 2, 4, 6 };
	a.merge(b);
	EXPECT_TRUE(b.empty());
	EXPECT_EQ(a, (std::list<int>{ 1, 2, 3, 4, 5, 6 }));

	std::list<int> donor{ 7, 8, 9 };
	a.splice(a.begin(), donor);
	EXPECT_TRUE(donor.empty());
	EXPECT_EQ(a.front(), 7);

	std::list<int> donor2{ 10, 11, 12 };
	auto pos = std::next(donor2.begin());  // points at 11
	a.splice(a.begin(), donor2, pos);
	EXPECT_EQ(a.front(), 11);
	EXPECT_EQ(donor2, (std::list<int>{ 10, 12 }));
}

TEST(ListHeader, SwapComparisonAndNonMemberEraseHelpers)
{
	// list supports lexical comparisons, swap, and C++20 erase helpers.
	std::list<int> a{ 1, 2, 3 };
	std::list<int> b{ 1, 2, 4 };
	EXPECT_TRUE(a < b);
	EXPECT_TRUE(a != b);

	a.swap(b);
	EXPECT_EQ(a, (std::list<int>{ 1, 2, 4 }));

	using std::swap;
	swap(a, b);
	EXPECT_EQ(a, (std::list<int>{ 1, 2, 3 }));

#ifdef __cpp_lib_erase_if
	std::list<int> c{ 1, 2, 2, 3, 4, 5, 6 };
	const auto removedEq = std::erase(c, 2);
	EXPECT_EQ(removedEq, 2u);
	const auto removedPred = std::erase_if(c, [](int v) { return (v % 2) == 0; });
	EXPECT_EQ(removedPred, 2u);  // removed 4 and 6
	EXPECT_EQ(c, (std::list<int>{ 1, 3, 5 }));
#endif
}

}  // namespace
#include <gtest/gtest.h>

#include <list>
#include <numeric>
#include <vector>

namespace {

	TEST(ListHeader, ConstructionAssignmentAndAccess)
	{
		std::list<int> a;
		EXPECT_TRUE(a.empty());

		std::list<int> b(3, 7);
		EXPECT_EQ(b.size(), 3u);
		EXPECT_EQ(b.front(), 7);
		EXPECT_EQ(b.back(), 7);

		std::vector<int> src{ 1, 2, 3 };
		std::list<int> c(src.begin(), src.end());
		EXPECT_EQ(c.size(), 3u);

		a.assign({ 4, 5, 6 });
		EXPECT_EQ(a.front(), 4);
		EXPECT_EQ(a.back(), 6);
	}

	TEST(ListHeader, InsertEraseSpliceMergeSortUniqueAndReverse)
	{
		std::list<int> l{ 3, 1, 2, 2, 4 };
		l.sort();
		l.unique();
		EXPECT_EQ((std::vector<int>(l.begin(), l.end())), (std::vector<int>{ 1, 2, 3, 4 }));

		auto it = std::next(l.begin(), 2);
		l.insert(it, 99);
		EXPECT_EQ((std::vector<int>(l.begin(), l.end())), (std::vector<int>{ 1, 2, 99, 3, 4 }));

		l.erase(std::next(l.begin(), 2));
		EXPECT_EQ((std::vector<int>(l.begin(), l.end())), (std::vector<int>{ 1, 2, 3, 4 }));

		std::list<int> other{ 5, 6 };
		l.splice(l.end(), other);
		EXPECT_TRUE(other.empty());
		EXPECT_EQ(l.back(), 6);

		std::list<int> m{ 0, 7 };
		l.merge(m);
		EXPECT_TRUE(m.empty());
		l.reverse();
		EXPECT_EQ(l.front(), 7);
	}

	TEST(ListHeader, RemoveOperationsAndEraseIf)
	{
		std::list<int> l{ 1, 2, 2, 3, 4, 5, 6 };
		l.remove(2);
		l.remove_if([](int v) { return (v % 2) == 0; });
		EXPECT_EQ((std::vector<int>(l.begin(), l.end())), (std::vector<int>{ 1, 3, 5 }));

#ifdef __cpp_lib_erase_if
		std::list<int> x{ 1, 1, 2, 3 };
		EXPECT_EQ(std::erase(x, 1), 2u);
		EXPECT_EQ(std::erase_if(x, [](int v) { return v > 2; }), 1u);
		EXPECT_EQ((std::vector<int>(x.begin(), x.end())), (std::vector<int>{ 2 }));
#endif
	}

}  // namespace
