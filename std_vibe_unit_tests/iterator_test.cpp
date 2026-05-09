#include <gtest/gtest.h>

#include <array>
#include <iterator>
#include <list>
#include <numeric>
#include <vector>

namespace {

TEST(IteratorHeader, IteratorTraitsAndCategoryChecks)
{
	// iterator_traits centralizes type information for generic iterator code.
	using Iter = std::vector<int>::iterator;
	EXPECT_TRUE((std::is_same_v<std::iterator_traits<Iter>::value_type, int>));
	EXPECT_TRUE((std::is_same_v<std::iterator_traits<Iter>::difference_type, std::ptrdiff_t>));
	EXPECT_TRUE((std::is_base_of_v<std::random_access_iterator_tag, std::iterator_traits<Iter>::iterator_category>));
}

TEST(IteratorHeader, DistanceNextPrevAndAdvanceUtilities)
{
	// distance/next/prev/advance are foundational non-member iterator utilities.
	std::list<int> values{ 10, 20, 30, 40 };
	auto begin = values.begin();
	auto end = values.end();
	EXPECT_EQ(std::distance(begin, end), 4);

	auto second = std::next(begin);
	EXPECT_EQ(*second, 20);
	auto first = std::prev(second);
	EXPECT_EQ(*first, 10);

	std::advance(begin, 3);
	EXPECT_EQ(*begin, 40);
}

TEST(IteratorHeader, InserterAdaptersWorkWithAlgorithms)
{
	// back/front/general inserters adapt algorithms to container insertion APIs.
	std::vector<int> src{ 1, 2, 3 };
	std::vector<int> dst;
	std::copy(src.begin(), src.end(), std::back_inserter(dst));
	EXPECT_EQ(dst, src);

	std::list<int> lst{ 2, 3 };
	std::copy(src.begin(), src.begin() + 1, std::front_inserter(lst));
	EXPECT_EQ(lst.front(), 1);
}

TEST(IteratorHeader, ReverseAndMoveIteratorAdapters)
{
	// reverse_iterator inverts traversal direction; make_move_iterator enables
	// move-based algorithm pipelines.
	std::array<int, 4> arr{ 1, 2, 3, 4 };
	std::vector<int> reversed;
	for (auto it = arr.rbegin(); it != arr.rend(); ++it)
	{
		reversed.push_back(*it);
	}
	EXPECT_EQ(reversed, (std::vector<int>{ 4, 3, 2, 1 }));

	std::vector<std::string> source{ "a", "b" };
	std::vector<std::string> moved;
	moved.insert(moved.end(),
		std::make_move_iterator(source.begin()),
		std::make_move_iterator(source.end()));
	EXPECT_EQ(moved, (std::vector<std::string>{ "a", "b" }));
}

TEST(IteratorHeader, RangeAccessHelpersBeginEndDataSize)
{
	// begin/end/data/size/empty free functions provide uniform range access.
	int raw[] = { 5, 6, 7 };
	EXPECT_EQ(*std::begin(raw), 5);
	EXPECT_EQ(*(std::end(raw) - 1), 7);
	EXPECT_EQ(std::size(raw), 3u);
	EXPECT_FALSE(std::empty(raw));
	EXPECT_EQ(*std::data(raw), 5);
}

}  // namespace
