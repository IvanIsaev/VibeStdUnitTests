#include <gtest/gtest.h>

#include <algorithm>
#include <deque>
#include <numeric>
#include <string>
#include <vector>

namespace {

	TEST(DequeHeader, ConstructorsAndAssignmentForms)
	{
		// std::deque supports default, count/value, range, copy, move, and
		// initializer-list construction plus assign() variants.
		std::deque<int> a;
		EXPECT_TRUE(a.empty());

		std::deque<int> b(4, 7);
		EXPECT_EQ(b.size(), 4u);
		EXPECT_EQ(b.front(), 7);

		std::vector<int> source{ 1, 2, 3 };
		std::deque<int> c(source.begin(), source.end());
		EXPECT_EQ(c, (std::deque<int>{ 1, 2, 3 }));

		std::deque<int> d{ 9, 8, 7 };
		std::deque<int> e(d);
		EXPECT_EQ(e, d);

		std::deque<int> f(std::move(e));
		EXPECT_EQ(f, (std::deque<int>{ 9, 8, 7 }));

		a.assign(3, 5);
		EXPECT_EQ(a, (std::deque<int>{ 5, 5, 5 }));
		a.assign(source.begin(), source.end());
		EXPECT_EQ(a, (std::deque<int>{ 1, 2, 3 }));
		a.assign({ 4, 5 });
		EXPECT_EQ(a, (std::deque<int>{ 4, 5 }));
	}

	TEST(DequeHeader, ElementAccessFrontBackSubscriptAndAt)
	{
		// deque exposes random-access element APIs similarly to vector.
		std::deque<std::string> dq{ "a", "b", "c" };
		EXPECT_EQ(dq.front(), "a");
		EXPECT_EQ(dq.back(), "c");
		EXPECT_EQ(dq[1], "b");
		EXPECT_EQ(dq.at(2), "c");
		EXPECT_THROW((void)dq.at(3), std::out_of_range);
	}

	TEST(DequeHeader, IteratorFamilyAndContiguousLikeRandomAccessOperations)
	{
		// deque iterators are random-access iterators with full begin/end/cbegin/
		// cend/rbegin/rend coverage.
		std::deque<int> dq{ 1, 2, 3, 4, 5 };

		const int sum = std::accumulate(dq.begin(), dq.end(), 0);
		EXPECT_EQ(sum, 15);

		auto it = dq.begin();
		it += 3;
		EXPECT_EQ(*it, 4);
		EXPECT_EQ(dq.end() - dq.begin(), 5);

		const std::deque<int>& cdq = dq;
		EXPECT_EQ(*cdq.cbegin(), 1);
		EXPECT_EQ(*cdq.crbegin(), 5);
	}

	TEST(DequeHeader, CapacityQueriesAndResizeSemantics)
	{
		// size/empty/max_size and resize variants govern capacity and element count.
		std::deque<int> dq;
		EXPECT_TRUE(dq.empty());
		EXPECT_EQ(dq.size(), 0u);
		EXPECT_GT(dq.max_size(), 0u);

		dq.resize(3);
		EXPECT_EQ(dq.size(), 3u);
		EXPECT_EQ(dq, (std::deque<int>{ 0, 0, 0 }));

		dq.resize(5, 9);
		EXPECT_EQ(dq, (std::deque<int>{ 0, 0, 0, 9, 9 }));

		dq.resize(2);
		EXPECT_EQ(dq, (std::deque<int>{ 0, 0 }));
	}

	TEST(DequeHeader, PushPopEmplaceAtBothEnds)
	{
		// deque specializes in efficient front/back insertion and removal.
		std::deque<int> dq;
		dq.push_back(2);
		dq.push_front(1);
		dq.emplace_back(3);
		dq.emplace_front(0);
		EXPECT_EQ(dq, (std::deque<int>{ 0, 1, 2, 3 }));

		dq.pop_front();
		dq.pop_back();
		EXPECT_EQ(dq, (std::deque<int>{ 1, 2 }));
	}

	TEST(DequeHeader, InsertEmplaceEraseAndClearOperations)
	{
		// Middle insertion/erasure APIs support single, multiple, range, and
		// initializer-list forms.
		std::deque<int> dq{ 1, 4 };

		auto it = dq.insert(dq.begin() + 1, 2);
		EXPECT_EQ(*it, 2);
		dq.insert(dq.begin() + 2, 1, 3);
		EXPECT_EQ(dq, (std::deque<int>{ 1, 2, 3, 4 }));

		std::vector<int> extra{ 5, 6 };
		dq.insert(dq.end(), extra.begin(), extra.end());
		EXPECT_EQ(dq, (std::deque<int>{ 1, 2, 3, 4, 5, 6 }));

		dq.insert(dq.begin(), { -1, 0 });
		EXPECT_EQ(dq, (std::deque<int>{ -1, 0, 1, 2, 3, 4, 5, 6 }));

		auto emplaced = dq.emplace(dq.begin() + 2, 99);
		EXPECT_EQ(*emplaced, 99);

		dq.erase(dq.begin() + 2); // erase 99
		dq.erase(dq.begin(), dq.begin() + 2); // erase -1,0
		EXPECT_EQ(dq, (std::deque<int>{ 1, 2, 3, 4, 5, 6 }));

		dq.clear();
		EXPECT_TRUE(dq.empty());
	}

	TEST(DequeHeader, SwapAndComparisonOperators)
	{
		// deque supports value-based comparisons and swap.
		std::deque<int> a{ 1, 2, 3 };
		std::deque<int> b{ 1, 2, 4 };
		EXPECT_TRUE(a < b);
		EXPECT_TRUE(a != b);

		a.swap(b);
		EXPECT_EQ(a, (std::deque<int>{ 1, 2, 4 }));
		EXPECT_EQ(b, (std::deque<int>{ 1, 2, 3 }));

		using std::swap;
		swap(a, b);
		EXPECT_EQ(a, (std::deque<int>{ 1, 2, 3 }));
	}

	TEST(DequeHeader, EraseAndEraseIfNonMemberHelpersWhenAvailable)
	{
		// C++20 adds std::erase/std::erase_if overloads for standard containers.
#ifdef __cpp_lib_erase_if
		EXPECT_GE(__cpp_lib_erase_if, 202002L);
		std::deque<int> dq{ 1, 2, 2, 3, 4, 5, 6 };
		const auto removedEq = std::erase(dq, 2);
		EXPECT_EQ(removedEq, 2u);
		EXPECT_EQ(dq, (std::deque<int>{ 1, 3, 4, 5, 6 }));

		const auto removedPred = std::erase_if(dq, [](int v) { return v % 2 == 0; });
		EXPECT_EQ(removedPred, 2u); // removed 4 and 6
		EXPECT_EQ(dq, (std::deque<int>{ 1, 3, 5 }));
#else
		GTEST_SKIP() << "std::erase/std::erase_if for deque unavailable.";
#endif
	}

	TEST(DequeHeader, AllocatorExposureAndGetAllocator)
	{
		// get_allocator exposes allocator instance used by deque.
		std::deque<int> dq{ 1, 2, 3 };
		auto alloc = dq.get_allocator();
		int* p = std::allocator_traits<decltype(alloc)>::allocate(alloc, 1);
		ASSERT_NE(p, nullptr);
		std::allocator_traits<decltype(alloc)>::construct(alloc, p, 123);
		EXPECT_EQ(*p, 123);
		std::allocator_traits<decltype(alloc)>::destroy(alloc, p);
		std::allocator_traits<decltype(alloc)>::deallocate(alloc, p, 1);
	}

}  // namespace
