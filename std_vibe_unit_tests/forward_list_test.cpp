#include <gtest/gtest.h>

#include <forward_list>
#include <numeric>
#include <string>
#include <vector>

namespace {

	TEST(ForwardList, ConstructorsAssignmentAndAllocatorAccess)
	{
		// forward_list supports default/count/range/copy/move/init-list construction
		// and assign() forms, while exposing allocator through get_allocator().
		std::forward_list<int> a;
		EXPECT_TRUE(a.empty());

		std::forward_list<int> b(3, 7);
		EXPECT_EQ(std::distance(b.begin(), b.end()), 3);

		std::vector<int> src{ 1, 2, 3 };
		std::forward_list<int> c(src.begin(), src.end());
		EXPECT_EQ(std::distance(c.begin(), c.end()), 3);

		std::forward_list<int> d{ 9, 8, 7 };
		std::forward_list<int> e(d);
		EXPECT_EQ(std::distance(e.begin(), e.end()), 3);

		std::forward_list<int> f(std::move(e));
		EXPECT_EQ(std::distance(f.begin(), f.end()), 3);

		a.assign(2, 5);
		EXPECT_EQ(std::distance(a.begin(), a.end()), 2);
		a.assign(src.begin(), src.end());
		EXPECT_EQ(std::distance(a.begin(), a.end()), 3);
		a.assign({ 4, 5 });
		EXPECT_EQ(std::distance(a.begin(), a.end()), 2);

		auto alloc = a.get_allocator();
		int* p = std::allocator_traits<decltype(alloc)>::allocate(alloc, 1);
		ASSERT_NE(p, nullptr);
		std::allocator_traits<decltype(alloc)>::deallocate(alloc, p, 1);
	}

	TEST(ForwardList, BeforeBeginFrontAndIteratorTraversal)
	{
		// forward_list provides singly-linked traversal with before_begin sentinel.
		std::forward_list<int> fl{ 1, 2, 3, 4 };
		EXPECT_EQ(fl.front(), 1);

		auto before = fl.before_begin();
		auto first = std::next(before);
		EXPECT_EQ(*first, 1);

		int sum = 0;
		for (auto it = fl.begin(); it != fl.end(); ++it)
		{
			sum += *it;
		}
		EXPECT_EQ(sum, 10);
	}

	TEST(ForwardList, EmptyMaxSizeClearAndResize)
	{
		// Capacity/state operations and resize behavior.
		std::forward_list<int> fl;
		EXPECT_TRUE(fl.empty());
		EXPECT_GT(fl.max_size(), 0u);

		fl.resize(3);
		EXPECT_EQ(std::distance(fl.begin(), fl.end()), 3);
		fl.resize(5, 9);
		EXPECT_EQ(std::distance(fl.begin(), fl.end()), 5);

		fl.clear();
		EXPECT_TRUE(fl.empty());
	}

	TEST(ForwardList, PushPopEmplaceAndInsertAfterFamily)
	{
		// forward_list inserts/erases primarily through *_after APIs plus front ops.
		std::forward_list<int> fl;
		fl.push_front(2);
		fl.emplace_front(1);
		EXPECT_EQ(fl.front(), 1);

		auto it = fl.before_begin();
		fl.insert_after(it, 0);
		EXPECT_EQ(fl.front(), 0);

		fl.insert_after(std::next(fl.begin()), 2, 9);
		std::vector<int> extra{ 7, 8 };
		fl.insert_after(fl.before_begin(), extra.begin(), extra.end());
		fl.insert_after(fl.before_begin(), { -1, -2 });

		// Ensure API calls were effective; exact shape is deterministic.
		std::vector<int> observed(fl.begin(), fl.end());
		EXPECT_FALSE(observed.empty());
		EXPECT_EQ(observed.front(), -1);

		fl.pop_front();
		EXPECT_EQ(fl.front(), -2);
	}

	TEST(ForwardList, EmplaceAfterAndEraseAfterVariants)
	{
		// emplace_after and erase_after are key singly-linked modifier operations.
		std::forward_list<std::string> fl{ "a", "c" };
		auto it = fl.begin();
		auto inserted = fl.emplace_after(it, "b");
		EXPECT_EQ(*inserted, "b");

		auto erasedNext = fl.erase_after(fl.begin()); // erase "b"
		EXPECT_EQ(*erasedNext, "c");
		EXPECT_EQ(std::distance(fl.begin(), fl.end()), 2);

		fl.insert_after(fl.before_begin(), { "x", "y", "z" });
		fl.erase_after(fl.before_begin(), std::next(fl.before_begin(), 3));
		EXPECT_EQ(fl.front(), "z");
	}

	TEST(ForwardList, RemoveRemoveIfUniqueAndReverse)
	{
		// Specialized list operations provide linear-time value/predicate removal,
		// adjacent duplicate elimination, and in-place reversal.
		std::forward_list<int> fl{ 1, 2, 2, 3, 4, 4, 4, 5 };
		fl.remove(2);
		fl.remove_if([](int v) { return v == 5; });
		fl.unique();
		fl.reverse();
		EXPECT_EQ((std::vector<int>(fl.begin(), fl.end())), (std::vector<int>{ 4, 3, 1 }));
	}

	TEST(ForwardList, SortMergeAndSpliceAfterOperations)
	{
		// sort/merge/splice_after are central linked-list algorithms and transfers.
		std::forward_list<int> a{ 5, 1, 3 };
		std::forward_list<int> b{ 6, 4, 2 };
		a.sort();
		b.sort();
		a.merge(b);
		EXPECT_TRUE(b.empty());
		EXPECT_EQ((std::vector<int>(a.begin(), a.end())), (std::vector<int>{ 1, 2, 3, 4, 5, 6 }));

		std::forward_list<int> donor{ 9, 8, 7 };
		a.splice_after(a.before_begin(), donor); // move all donor to front of a
		EXPECT_TRUE(donor.empty());
		EXPECT_EQ(a.front(), 9);

		// Single-element splice_after from middle.
		std::forward_list<int> src{ 10, 11, 12 };
		auto beforeElement = src.begin(); // before 11
		a.splice_after(a.before_begin(), src, beforeElement); // move 11
		EXPECT_EQ(a.front(), 11);
	}

	TEST(ForwardList, SwapAndComparisonOperators)
	{
		// forward_list supports value comparisons and swap.
		std::forward_list<int> a{ 1, 2, 3 };
		std::forward_list<int> b{ 1, 2, 4 };
		EXPECT_TRUE(a < b);
		EXPECT_TRUE(a != b);

		a.swap(b);
		EXPECT_EQ((std::vector<int>(a.begin(), a.end())), (std::vector<int>{ 1, 2, 4 }));
		EXPECT_EQ((std::vector<int>(b.begin(), b.end())), (std::vector<int>{ 1, 2, 3 }));

		using std::swap;
		swap(a, b);
		EXPECT_EQ((std::vector<int>(a.begin(), a.end())), (std::vector<int>{ 1, 2, 3 }));
	}

	TEST(ForwardList, EraseAndEraseIfNonMemberHelpersWhenAvailable)
	{
		// C++20 non-member erase helpers for forward_list.
#ifdef __cpp_lib_erase_if
		std::forward_list<int> fl{ 1, 2, 2, 3, 4, 5, 6 };
		const auto removedEq = std::erase(fl, 2);
		EXPECT_EQ(removedEq, 2u);
		const auto removedPred = std::erase_if(fl, [](int v) { return (v % 2) == 0; });
		EXPECT_EQ(removedPred, 2u); // removed 4 and 6
		EXPECT_EQ((std::vector<int>(fl.begin(), fl.end())), (std::vector<int>{ 1, 3, 5 }));
#else
		GTEST_SKIP() << "std::erase/std::erase_if unavailable.";
#endif
	}

}  // namespace
