#include <gtest/gtest.h>

#include <initializer_list>
#include <iterator>
#include <numeric>
#include <type_traits>

namespace {

	TEST(InitializerList, TypeAliasesExposeConstElementView)
	{
		// std::initializer_list<T> is a lightweight proxy over a compiler-managed
		// array of const T objects. Its public aliases are part of the header's API
		// contract and describe read-only access semantics used by generic code.
		using List = std::initializer_list<int>;

		EXPECT_TRUE((std::is_same_v<List::value_type, int>));
		EXPECT_TRUE((std::is_same_v<List::reference, const int&>));
		EXPECT_TRUE((std::is_same_v<List::const_reference, const int&>));
		EXPECT_TRUE((std::is_same_v<List::iterator, const int*>));
		EXPECT_TRUE((std::is_same_v<List::const_iterator, const int*>));
		EXPECT_TRUE((std::is_same_v<List::size_type, std::size_t>));
	}

	TEST(InitializerList, DefaultConstructedListIsEmpty)
	{
		// A value-initialized initializer_list has no backing elements. The standard
		// API should report size zero and equal begin/end iterators for this state.
		const std::initializer_list<int> values{};

		EXPECT_EQ(values.size(), 0u);
		EXPECT_EQ(values.begin(), values.end());
	}

	TEST(InitializerList, BraceConstructedListPreservesSequenceAndIteration)
	{
		// Brace syntax materializes an initializer list sequence. begin()/end() must
		// iterate the elements in source order, and size() reports the element count.
		const std::initializer_list<int> values = { 4, 8, 15, 16, 23, 42 };

		ASSERT_EQ(values.size(), 6u);
		EXPECT_EQ(*values.begin(), 4);
		EXPECT_EQ(*(values.end() - 1), 42);
		EXPECT_EQ(std::distance(values.begin(), values.end()), 6);

		const int sum = std::accumulate(values.begin(), values.end(), 0);
		EXPECT_EQ(sum, 108);
	}

	TEST(InitializerList, CopyConstructedListViewsSameElements)
	{
		// Copying std::initializer_list copies only the lightweight view metadata,
		// not the underlying elements. Therefore both lists should expose identical
		// size and iteration range over the same immutable sequence.
		const std::initializer_list<int> original = { 1, 2, 3, 5, 8 };
		const std::initializer_list<int> copied = original;

		EXPECT_EQ(copied.size(), original.size());
		EXPECT_EQ(copied.begin(), original.begin());
		EXPECT_EQ(copied.end(), original.end());
		EXPECT_EQ(std::accumulate(copied.begin(), copied.end(), 0), 19);
	}

	TEST(InitializerList, AssignmentRebindsViewToNewSequence)
	{
		// Assignment updates which underlying sequence the view references. This
		// test ensures reassignment changes observed size/content as expected and
		// keeps the interface purely read-only.
		std::initializer_list<int> values = { 10, 20 };
		EXPECT_EQ(values.size(), 2u);
		EXPECT_EQ(std::accumulate(values.begin(), values.end(), 0), 30);

		values = { 7, 11, 13 };
		EXPECT_EQ(values.size(), 3u);
		EXPECT_EQ(std::accumulate(values.begin(), values.end(), 0), 31);
		EXPECT_EQ(*values.begin(), 7);
	}

	TEST(InitializerList, IteratorCategorySupportsRangeForAndConstAccess)
	{
		// Iterators from std::initializer_list are const pointers, so range-for loops
		// provide read-only element access. This test validates practical iteration
		// behavior and that dereferenced values match the declared sequence.
		const std::initializer_list<int> values = { 3, 6, 9 };
		int visitedCount = 0;
		int visitedProduct = 1;
		for (const int value : values)
		{
			visitedProduct *= value;
			++visitedCount;
		}

		EXPECT_EQ(visitedCount, 3);
		EXPECT_EQ(visitedProduct, 162);
	}

}  // namespace
