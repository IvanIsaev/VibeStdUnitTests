#include <gtest/gtest.h>

#include <algorithm>
#include <compare>
#include <functional>
#include <iterator>
#include <numeric>
#include <random>
#include <utility>
#include <vector>


namespace {

	TEST(Algorithms, AllOf)
	{
		// all_of: true only if the predicate holds for every element (short-circuits on first false).
		std::vector<int> v{ 2, 4, 6 };
		EXPECT_TRUE(std::all_of(v.begin(), v.end(), [](int x) { return x % 2 == 0; }));
		v.push_back(3);
		EXPECT_FALSE(std::all_of(v.begin(), v.end(), [](int x) { return x % 2 == 0; }));
	}

	TEST(Algorithms, AnyOf)
	{
		// any_of: true if the predicate holds for at least one element (short-circuits on first true).
		std::vector<int> v{ 1, 3, 4 };
		EXPECT_TRUE(std::any_of(v.begin(), v.end(), [](int x) { return x % 2 == 0; }));
		EXPECT_FALSE(std::any_of(v.begin(), v.end(), [](int x) { return x > 10; }));
	}

	TEST(Algorithms, NoneOf)
	{
		// none_of: true if the predicate is false for every element (short-circuits on first true predicate).
		std::vector<int> v{ 1, 3, 5 };
		EXPECT_TRUE(std::none_of(v.begin(), v.end(), [](int x) { return x % 2 == 0; }));
		v.push_back(2);
		EXPECT_FALSE(std::none_of(v.begin(), v.end(), [](int x) { return x % 2 == 0; }));
	}

	TEST(Algorithms, ForEach)
	{
		// for_each: applies a function object to each element; return value is the function (often ignored).
		std::vector<int> v{ 1, 2, 3 };
		int sum = 0;
		std::for_each(v.begin(), v.end(), [&sum](int x) { sum += x; });
		EXPECT_EQ(sum, 6);
	}

	TEST(Algorithms, ForEachN)
	{
		// for_each_n: like for_each but only the first n elements (C++17).
		std::vector<int> v{ 10, 20, 30, 40 };
		int sum = 0;
		std::for_each_n(v.begin(), 2, [&sum](int x) { sum += x; });
		EXPECT_EQ(sum, 30);
	}

	TEST(Algorithms, Count)
	{
		// count: number of elements equal to a given value.
		std::vector<int> v{ 1, 2, 2, 3, 2 };
		EXPECT_EQ(std::count(v.begin(), v.end(), 2), 3);
	}

	TEST(Algorithms, CountIf)
	{
		// count_if: number of elements satisfying a predicate.
		std::vector<int> v{ 1, 2, 3, 4, 5 };
		EXPECT_EQ(std::count_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; }), 2);
	}

	TEST(Algorithms, Mismatch)
	{
		// mismatch: first pair of positions where two ranges differ (or ends if prefixes match).
		std::vector<int> a{ 1, 2, 3 }, b{ 1, 9, 3 };
		auto [it1, it2] = std::mismatch(a.begin(), a.end(), b.begin(), b.end());
		EXPECT_EQ(*it1, 2);
		EXPECT_EQ(*it2, 9);
	}

	TEST(Algorithms, Find)
	{
		// find: iterator to the first element equal to value, or end if not found.
		std::vector<int> v{ 1, 2, 3 };
		auto it = std::find(v.begin(), v.end(), 2);
		ASSERT_NE(it, v.end());
		EXPECT_EQ(*it, 2);
	}

	TEST(Algorithms, FindIf)
	{
		// find_if: iterator to the first element satisfying the predicate.
		std::vector<int> v{ 1, 2, 3 };
		auto it = std::find_if(v.begin(), v.end(), [](int x) { return x > 1; });
		ASSERT_NE(it, v.end());
		EXPECT_EQ(*it, 2);
	}

	TEST(Algorithms, FindIfNot)
	{
		// find_if_not: iterator to the first element for which the predicate is false.
		std::vector<int> v{ 2, 4, 5, 6 };
		auto it = std::find_if_not(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
		ASSERT_NE(it, v.end());
		EXPECT_EQ(*it, 5);
	}

	TEST(Algorithms, FindEnd)
	{
		// find_end: start of the last occurrence of subsequence [needle) inside [haystack).
		std::vector<int> h{ 1, 2, 1, 2, 3 }, n{ 1, 2 };
		auto it = std::find_end(h.begin(), h.end(), n.begin(), n.end());
		ASSERT_NE(it, h.end());
		EXPECT_EQ(it - h.begin(), 2);
	}

	TEST(Algorithms, FindFirstOf)
	{
		// find_first_of: first element in the first range that matches any element in the second range.
		std::vector<int> h{ 0, 0, 3, 0 }, need{ 9, 3, 1 };
		auto it = std::find_first_of(h.begin(), h.end(), need.begin(), need.end());
		ASSERT_NE(it, h.end());
		EXPECT_EQ(*it, 3);
	}

	TEST(Algorithms, AdjacentFind)
	{
		// adjacent_find: first element whose successor compares equal (default: duplicates).
		std::vector<int> v{ 1, 2, 2, 3 };
		auto it = std::adjacent_find(v.begin(), v.end());
		ASSERT_NE(it, v.end());
		EXPECT_EQ(*it, 2);
	}

	TEST(Algorithms, Search)
	{
		// search: first position where subsequence [needle) occurs as a contiguous subrange.
		std::vector<int> h{ 1, 2, 3, 4, 5 }, n{ 3, 4 };
		auto it = std::search(h.begin(), h.end(), n.begin(), n.end());
		ASSERT_NE(it, h.end());
		EXPECT_EQ(it - h.begin(), 2);
	}

	TEST(Algorithms, SearchN)
	{
		// search_n: first position of n consecutive elements each equal to a given value.
		std::vector<int> v{ 1, 2, 2, 2, 3 };
		auto it = std::search_n(v.begin(), v.end(), 3, 2);
		ASSERT_NE(it, v.end());
		EXPECT_EQ(it - v.begin(), 1);
	}

	TEST(Algorithms, Copy)
	{
		// copy: assigns each element from [first, last) to the output sequence starting at d_first.
		std::vector<int> src{ 1, 2, 3 }, dst(3);
		std::copy(src.begin(), src.end(), dst.begin());
		EXPECT_EQ(dst, src);
	}

	TEST(Algorithms, CopyIf)
	{
		// copy_if: copies only elements satisfying the predicate (output must not overlap input in general).
		std::vector<int> src{ 1, 2, 3, 4 }, dst;
		std::copy_if(src.begin(), src.end(), std::back_inserter(dst), [](int x) { return x % 2 == 0; });
		EXPECT_EQ(dst, (std::vector<int>{2, 4}));
	}

	TEST(Algorithms, CopyN)
	{
		// copy_n: copies exactly n values from first onward; n must not exceed the readable source length (otherwise UB).
		std::vector<int> src{ 9, 8, 7 }, dst(2);
		std::copy_n(src.begin(), 2, dst.begin());
		EXPECT_EQ(dst, (std::vector<int>{9, 8}));
	}

	TEST(Algorithms, CopyBackward)
	{
		// copy_backward: like copy but suitable when [d_first, d_first+(last-first)) overlaps the end of the source range.
		std::vector<int> v{ 1, 2, 3, 0, 0 };
		std::copy_backward(v.begin(), v.begin() + 3, v.end());
		EXPECT_EQ(v, (std::vector<int>{1, 2, 1, 2, 3}));
	}

	TEST(Algorithms, Move)
	{
		// move: moves elements from source range to output (typically via std::move on each element).
		std::vector<std::unique_ptr<int>> src;
		src.push_back(std::make_unique<int>(1));
		src.push_back(std::make_unique<int>(2));
		std::vector<std::unique_ptr<int>> dst(2);
		std::move(src.begin(), src.end(), dst.begin());
		ASSERT_TRUE(dst[0]);
		EXPECT_EQ(*dst[0], 1);
		EXPECT_EQ(*dst[1], 2);
	}

	TEST(Algorithms, MoveBackward)
	{
		// move_backward: moves right-to-left for correct overlap when destination is right of source start.
		std::vector<std::unique_ptr<int>> v(3);
		v[0] = std::make_unique<int>(1);
		std::move_backward(v.begin(), v.begin() + 1, v.end());
		EXPECT_FALSE(v[0]);
		ASSERT_TRUE(v[2]);
		EXPECT_EQ(*v[2], 1);
	}

	TEST(Algorithms, Fill)
	{
		// fill: assigns the same value to every element in [first, last).
		std::vector<int> v(4);
		std::fill(v.begin(), v.end(), 7);
		EXPECT_EQ(v, (std::vector<int>{7, 7, 7, 7}));
	}

	TEST(Algorithms, FillN)
	{
		// fill_n: assigns value to n successive output positions (often used with back_inserter).
		std::vector<int> v;
		std::fill_n(std::back_inserter(v), 3, -1);
		EXPECT_EQ(v, (std::vector<int>{-1, -1, -1}));
	}

	TEST(Algorithms, Transform)
	{
		// transform (binary): writes op(a_i, b_i) for parallel input ranges (requires compatible lengths).
		std::vector<int> a{ 1, 2, 3 }, b{ 10, 20, 30 }, out(3);
		std::transform(a.begin(), a.end(), b.begin(), out.begin(), std::plus{});
		EXPECT_EQ(out, (std::vector<int>{11, 22, 33}));
	}

	TEST(Algorithms, TransformUnary)
	{
		// transform (unary): writes op(*it) for each element of a single input range.
		std::vector<int> v{ 1, 2, 3 }, out(3);
		std::transform(v.begin(), v.end(), out.begin(), [](int x) { return x * x; });
		EXPECT_EQ(out, (std::vector<int>{1, 4, 9}));
	}

	TEST(Algorithms, Generate)
	{
		// generate: fills [first, last) by repeatedly invoking the generator (no arguments).
		std::vector<int> v(3);
		int n = 0;
		std::generate(v.begin(), v.end(), [&n] { return ++n; });
		EXPECT_EQ(v, (std::vector<int>{1, 2, 3}));
	}

	TEST(Algorithms, GenerateN)
	{
		// generate_n: invokes the generator n times and writes each result to successive output positions.
		std::vector<int> v;
		int n = 0;
		std::generate_n(std::back_inserter(v), 4, [&n] { return n++ * 2; });
		EXPECT_EQ(v, (std::vector<int>{0, 2, 4, 6}));
	}

	TEST(Algorithms, Remove)
	{
		// remove: moves non-removed elements forward; returns new logical end — caller must erase [new_end, end).
		std::vector<int> v{ 1, 2, 3, 2, 4 };
		auto it = std::remove(v.begin(), v.end(), 2);
		v.erase(it, v.end());
		EXPECT_EQ(v, (std::vector<int>{1, 3, 4}));
	}

	TEST(Algorithms, RemoveIf)
	{
		// remove_if: same as remove but removes elements satisfying the predicate.
		std::vector<int> v{ 1, 2, 3, 4 };
		auto it = std::remove_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
		v.erase(it, v.end());
		EXPECT_EQ(v, (std::vector<int>{1, 3}));
	}

	TEST(Algorithms, RemoveCopy)
	{
		// remove_copy: copies all elements not equal to value (source unchanged).
		std::vector<int> src{ 1, 2, 2, 3 }, dst;
		std::remove_copy(src.begin(), src.end(), std::back_inserter(dst), 2);
		EXPECT_EQ(dst, (std::vector<int>{1, 3}));
	}

	TEST(Algorithms, RemoveCopyIf)
	{
		// remove_copy_if: copies elements for which the predicate is false.
		std::vector<int> src{ 1, 2, 3, 4 }, dst;
		std::remove_copy_if(src.begin(), src.end(), std::back_inserter(dst), [](int x) { return x % 2 == 0; });
		EXPECT_EQ(dst, (std::vector<int>{1, 3}));
	}

	TEST(Algorithms, Replace)
	{
		// replace: assigns new_value to every element that compares equal to old_value.
		std::vector<int> v{ 1, 2, 2, 3 };
		std::replace(v.begin(), v.end(), 2, 9);
		EXPECT_EQ(v, (std::vector<int>{1, 9, 9, 3}));
	}

	TEST(Algorithms, ReplaceIf)
	{
		// replace_if: assigns new_value wherever the predicate holds.
		std::vector<int> v{ 1, 2, 3, 4 };
		std::replace_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; }, 0);
		EXPECT_EQ(v, (std::vector<int>{1, 0, 3, 0}));
	}

	TEST(Algorithms, ReplaceCopy)
	{
		// replace_copy: writes a copy of the range with old_value replaced by new_value.
		std::vector<int> src{ 1, 2, 2 }, dst;
		std::replace_copy(src.begin(), src.end(), std::back_inserter(dst), 2, 9);
		EXPECT_EQ(dst, (std::vector<int>{1, 9, 9}));
	}

	TEST(Algorithms, ReplaceCopyIf)
	{
		// replace_copy_if: output is like input but with new_value where predicate is true.
		std::vector<int> src{ 1, 2, 3, 4 }, dst;
		std::replace_copy_if(
			src.begin(), src.end(), std::back_inserter(dst), [](int x) { return x % 2 == 0; }, 0);
		EXPECT_EQ(dst, (std::vector<int>{1, 0, 3, 0}));
	}

	TEST(Algorithms, SwapRanges)
	{
		// swap_ranges: exchanges corresponding pairs of elements in two ranges of equal length.
		std::vector<int> a{ 1, 2, 3 }, b{ 9, 8, 7 };
		std::swap_ranges(a.begin(), a.end(), b.begin());
		EXPECT_EQ(a, (std::vector<int>{9, 8, 7}));
		EXPECT_EQ(b, (std::vector<int>{1, 2, 3}));
	}

	TEST(Algorithms, Reverse)
	{
		// reverse: reverses element order in place (bidirectional iterators).
		std::vector<int> v{ 1, 2, 3 };
		std::reverse(v.begin(), v.end());
		EXPECT_EQ(v, (std::vector<int>{3, 2, 1}));
	}

	TEST(Algorithms, ReverseCopy)
	{
		// reverse_copy: writes the reversed sequence to another range (source unchanged).
		std::vector<int> src{ 1, 2, 3 }, dst(3);
		std::reverse_copy(src.begin(), src.end(), dst.begin());
		EXPECT_EQ(dst, (std::vector<int>{3, 2, 1}));
	}

	TEST(Algorithms, Rotate)
	{
		// rotate: permutes so that element at middle becomes first (left part wraps to end).
		std::vector<int> v{ 1, 2, 3, 4, 5 };
		auto mid = v.begin() + 2;
		std::rotate(v.begin(), mid, v.end());
		EXPECT_EQ(v, (std::vector<int>{3, 4, 5, 1, 2}));
	}

	TEST(Algorithms, RotateCopy)
	{
		// rotate_copy: output is the rotation of [first, last) around middle (no in-place mutation).
		std::vector<int> src{ 1, 2, 3, 4 }, dst(4);
		std::rotate_copy(src.begin(), src.begin() + 1, src.end(), dst.begin());
		EXPECT_EQ(dst, (std::vector<int>{2, 3, 4, 1}));
	}

	TEST(Algorithms, ShiftLeft)
	{
		// shift_left (C++20): moves elements n positions toward the front; returns end of valid tail to erase.
		std::vector<int> v{ 1, 2, 3, 4, 5 };
		auto it = std::shift_left(v.begin(), v.end(), 2);
		v.erase(it, v.end());
		EXPECT_EQ(v, (std::vector<int>{3, 4, 5}));
	}

	TEST(Algorithms, ShiftRight)
	{
		// shift_right (C++20): moves elements toward the end opening n slots at the front; erase junk prefix via returned iterator.
		std::vector<int> v{ 1, 2, 3, 4, 5 };
		auto it = std::shift_right(v.begin(), v.end(), 2);
		v.erase(v.begin(), it);
		EXPECT_EQ(v, (std::vector<int>{1, 2, 3}));
	}

	TEST(Algorithms, Shuffle)
	{
		// shuffle: randomly permutes using the given uniform random bit generator.
		std::vector<int> v(20);
		std::iota(v.begin(), v.end(), 0);
		std::vector<int> before = v;
		std::mt19937 gen{42};
		std::shuffle(v.begin(), v.end(), gen);
		EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), std::accumulate(before.begin(), before.end(), 0));
		EXPECT_NE(v, before);
	}

	TEST(Algorithms, Sample)
	{
		// sample: selects at most n random distinct elements (without replacement when using forward iterators).
		std::vector<int> src{ 1, 2, 3, 4, 5 }, dst;
		std::mt19937 gen{123};
		std::sample(src.begin(), src.end(), std::back_inserter(dst), 3, gen);
		EXPECT_EQ(dst.size(), 3u);
		for (int x : dst)
			EXPECT_NE(std::find(src.begin(), src.end(), x), src.end());
	}

	TEST(Algorithms, Unique)
	{
		// unique: collapses consecutive duplicates (range should often be sorted first for global uniqueness).
		std::vector<int> v{ 1, 1, 2, 2, 2, 3, 3 };
		auto it = std::unique(v.begin(), v.end());
		v.erase(it, v.end());
		EXPECT_EQ(v, (std::vector<int>{1, 2, 3}));
	}

	TEST(Algorithms, UniqueCopy)
	{
		// unique_copy: writes one copy of each maximal run of equal consecutive elements.
		std::vector<int> src{ 1, 1, 2, 2, 3 }, dst;
		std::unique_copy(src.begin(), src.end(), std::back_inserter(dst));
		EXPECT_EQ(dst, (std::vector<int>{1, 2, 3}));
	}

	TEST(Algorithms, IsPartitioned)
	{
		// is_partitioned: true iff all predicate-true elements precede all predicate-false elements.
		std::vector<int> ok{ 4, 2, 6, 1, 3 }, bad{ 1, 4, 2 };
		EXPECT_TRUE(std::is_partitioned(ok.begin(), ok.end(), [](int x) { return x % 2 == 0; }));
		EXPECT_FALSE(std::is_partitioned(bad.begin(), bad.end(), [](int x) { return x % 2 == 0; }));
	}

	TEST(Algorithms, Partition)
	{
		// partition: reorders so predicate-true elements come first; returns split iterator (not stable).
		std::vector<int> v{ 1, 2, 3, 4, 5 };
		auto mid = std::partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
		EXPECT_TRUE(std::all_of(v.begin(), mid, [](int x) { return x % 2 == 0; }));
		EXPECT_TRUE(std::none_of(mid, v.end(), [](int x) { return x % 2 == 0; }));
	}

	TEST(Algorithms, StablePartition)
	{
		// stable_partition: like partition but preserves relative order within each group.
		std::vector<std::pair<int, int>> v{ {2, 0}, {1, 1}, {4, 2}, {3, 3} };
		std::stable_partition(v.begin(), v.end(), [](const auto& p) { return p.first % 2 == 0; });
		EXPECT_EQ(v[0].second, 0);
		EXPECT_EQ(v[1].second, 2);
	}

	TEST(Algorithms, PartitionPoint)
	{
		// partition_point: first position where predicate is false, assuming range is already partitioned by that predicate.
		std::vector<int> v{ 2, 4, 6, 1, 3 };
		auto mid = std::partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
		auto pp = std::partition_point(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
		EXPECT_EQ(pp, mid);
	}

	TEST(Algorithms, IsSorted)
	{
		// is_sorted: true if each element is <= the next (non-decreasing by default comparator).
		std::vector<int> a{ 1, 2, 3 }, b{ 2, 1, 3 };
		EXPECT_TRUE(std::is_sorted(a.begin(), a.end()));
		EXPECT_FALSE(std::is_sorted(b.begin(), b.end()));
	}

	TEST(Algorithms, IsSortedUntil)
	{
		// is_sorted_until: iterator to first element that breaks sorted order (or end if fully sorted).
		std::vector<int> v{ 1, 2, 2, 1, 3 };
		auto it = std::is_sorted_until(v.begin(), v.end());
		EXPECT_EQ(it - v.begin(), 3);
	}

	TEST(Algorithms, Sort)
	{
		// sort: sorts in place (typically O(n log n); not stable).
		std::vector<int> v{ 3, 1, 4, 1, 5 };
		std::sort(v.begin(), v.end());
		EXPECT_EQ(v, (std::vector<int>{1, 1, 3, 4, 5}));
	}

	TEST(Algorithms, StableSort)
	{
		// stable_sort: sort preserving relative order of equivalent elements.
		std::vector<std::pair<int, int>> v{ {1, 0}, {1, 1}, {0, 2} };
		std::stable_sort(v.begin(), v.end());
		EXPECT_EQ(v[0].second, 2);
		EXPECT_EQ(v[1].second, 0);
		EXPECT_EQ(v[2].second, 1);
	}

	TEST(Algorithms, PartialSort)
	{
		// partial_sort: places the (middle - first) smallest elements, sorted, into [first, middle); the rest is unspecified.
		std::vector<int> v{ 7, 1, 9, 3, 2 };
		std::partial_sort(v.begin(), v.begin() + 3, v.end());
		std::vector<int> head(v.begin(), v.begin() + 3);
		std::sort(head.begin(), head.end());
		EXPECT_EQ(head, (std::vector<int>{1, 2, 3}));
	}

	TEST(Algorithms, PartialSortCopy)
	{
		// partial_sort_copy: fills the output with the n smallest elements from the input in sorted order (n = output size).
		std::vector<int> src{ 9, 1, 8, 2, 7 }, dst(3);
		std::partial_sort_copy(src.begin(), src.end(), dst.begin(), dst.end());
		EXPECT_EQ(dst, (std::vector<int>{1, 2, 7}));
	}

	TEST(Algorithms, NthElement)
	{
		// nth_element: partitions so nth is in sorted position; elements before/after are unspecified except relative to nth.
		std::vector<int> v{ 5, 1, 9, 3, 7 };
		auto nth = v.begin() + 2;
		std::nth_element(v.begin(), nth, v.end());
		EXPECT_EQ(*nth, 5);
	}

	TEST(Algorithms, LowerBound)
	{
		// lower_bound: first position where value could be inserted without breaking sorted order (binary search).
		std::vector<int> v{ 1, 2, 2, 2, 3 };
		auto it = std::lower_bound(v.begin(), v.end(), 2);
		EXPECT_EQ(it - v.begin(), 1);
	}

	TEST(Algorithms, UpperBound)
	{
		// upper_bound: first position after all elements equal to value in a sorted range.
		std::vector<int> v{ 1, 2, 2, 2, 3 };
		auto it = std::upper_bound(v.begin(), v.end(), 2);
		EXPECT_EQ(it - v.begin(), 4);
	}

	TEST(Algorithms, BinarySearch)
	{
		// binary_search: true if sorted range contains an element equal to value (uses comparisons).
		std::vector<int> v{ 1, 3, 5, 7 };
		EXPECT_TRUE(std::binary_search(v.begin(), v.end(), 5));
		EXPECT_FALSE(std::binary_search(v.begin(), v.end(), 4));
	}

	TEST(Algorithms, EqualRange)
	{
		// equal_range: pair [lower_bound, upper_bound) — the subrange of elements equal to value.
		std::vector<int> v{ 1, 2, 2, 2, 4 };
		auto [lo, hi] = std::equal_range(v.begin(), v.end(), 2);
		EXPECT_EQ(lo - v.begin(), 1);
		EXPECT_EQ(hi - v.begin(), 4);
	}

	TEST(Algorithms, Merge)
	{
		// merge: combines two sorted ranges into one sorted output (linear if inputs sorted).
		std::vector<int> a{ 1, 3, 5 }, b{ 2, 4, 6 }, out;
		std::merge(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
		EXPECT_EQ(out, (std::vector<int>{1, 2, 3, 4, 5, 6}));
	}

	TEST(Algorithms, InplaceMerge)
	{
		// inplace_merge: merges two consecutive sorted subranges [first, middle) and [middle, last) in place.
		std::vector<int> v{ 1, 3, 5, 2, 4, 6 };
		auto mid = v.begin() + 3;
		std::inplace_merge(v.begin(), mid, v.end());
		EXPECT_EQ(v, (std::vector<int>{1, 2, 3, 4, 5, 6}));
	}

	TEST(Algorithms, Includes)
	{
		// includes: true if every element of sorted multiset [2) occurs in sorted multiset [1) with sufficient multiplicity.
		std::vector<int> a{ 1, 2, 2, 3, 4 }, b{ 2, 2, 4 };
		EXPECT_TRUE(std::includes(a.begin(), a.end(), b.begin(), b.end()));
		std::vector<int> c{ 2, 5 };
		EXPECT_FALSE(std::includes(a.begin(), a.end(), c.begin(), c.end()));
	}

	TEST(Algorithms, SetDifference)
	{
		// set_difference: sorted multiset difference (copies elements in 1 not “covered” by 2).
		std::vector<int> a{ 1, 2, 2, 3 }, b{ 2, 4 }, out;
		std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
		EXPECT_EQ(out, (std::vector<int>{1, 2, 3}));
	}

	TEST(Algorithms, SetIntersection)
	{
		// set_intersection: sorted multiset intersection written in order to output.
		std::vector<int> a{ 1, 2, 2, 3 }, b{ 2, 2, 4 }, out;
		std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
		EXPECT_EQ(out, (std::vector<int>{2, 2}));
	}

	TEST(Algorithms, SetSymmetricDifference)
	{
		// set_symmetric_difference: elements that appear in exactly one of the two sorted multisets.
		std::vector<int> a{ 1, 2, 3 }, b{ 2, 3, 4 }, out;
		std::set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
		EXPECT_EQ(out, (std::vector<int>{1, 4}));
	}

	TEST(Algorithms, SetUnion)
	{
		// set_union: merges two sorted multisets into sorted multiset output (stable merge of equals).
		std::vector<int> a{ 1, 2, 2 }, b{ 2, 3 }, out;
		std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
		EXPECT_EQ(out, (std::vector<int>{1, 2, 2, 3}));
	}

	TEST(Algorithms, PushHeap)
	{
		// push_heap: assumes [first, last-1) is a heap; sifts the last element into place so [first, last) is a heap.
		std::vector<int> h{ 3, 1, 4 };
		std::make_heap(h.begin(), h.end());
		h.push_back(9);
		std::push_heap(h.begin(), h.end());
		EXPECT_EQ(h.front(), 9);
	}

	TEST(Algorithms, PopHeap)
	{
		// pop_heap: moves the largest (heap root) to last-1 and restores heap on [first, last-1).
		std::vector<int> h{ 9, 5, 7, 1 };
		std::make_heap(h.begin(), h.end());
		std::pop_heap(h.begin(), h.end());
		EXPECT_EQ(h.back(), 9);
		h.pop_back();
		EXPECT_TRUE(std::is_heap(h.begin(), h.end()));
	}

	TEST(Algorithms, MakeHeap)
	{
		// make_heap: rearranges into max-heap so comp(parent, child) is false for heap property (default <).
		std::vector<int> h{ 3, 1, 4, 1, 5 };
		std::make_heap(h.begin(), h.end());
		EXPECT_TRUE(std::is_heap(h.begin(), h.end()));
	}

	TEST(Algorithms, SortHeap)
	{
		// sort_heap: repeatedly pop_heap — turns a heap into a sorted ascending range (invalidates heap afterward).
		std::vector<int> h{ 3, 1, 4, 1, 5 };
		std::make_heap(h.begin(), h.end());
		std::sort_heap(h.begin(), h.end());
		EXPECT_EQ(h, (std::vector<int>{1, 1, 3, 4, 5}));
	}

	TEST(Algorithms, IsHeap)
	{
		// is_heap: true if the range satisfies the binary heap property for the given comparator.
		std::vector<int> ok{ 9, 5, 7 }, bad{ 1, 9, 5 };
		EXPECT_TRUE(std::is_heap(ok.begin(), ok.end()));
		EXPECT_FALSE(std::is_heap(bad.begin(), bad.end()));
	}

	TEST(Algorithms, IsHeapUntil)
	{
		// is_heap_until: first position where heap property fails (or end if entire range is a heap).
		std::vector<int> v{ 9, 5, 1, 7 };
		auto it = std::is_heap_until(v.begin(), v.end());
		EXPECT_EQ(it - v.begin(), 3);
	}

	TEST(Algorithms, MaxElement)
	{
		// max_element: iterator to the largest element (first such if ties, per comparator).
		std::vector<int> v{ 1, 9, 3 };
		auto it = std::max_element(v.begin(), v.end());
		ASSERT_NE(it, v.end());
		EXPECT_EQ(*it, 9);
	}

	TEST(Algorithms, MinElement)
	{
		// min_element: iterator to the smallest element (first such if ties).
		std::vector<int> v{ 4, 2, 7 };
		auto it = std::min_element(v.begin(), v.end());
		ASSERT_NE(it, v.end());
		EXPECT_EQ(*it, 2);
	}

	TEST(Algorithms, MinmaxElement)
	{
		// minmax_element: pair of iterators to smallest and largest in one scan.
		std::vector<int> v{ 3, 1, 4 };
		auto [mn, mx] = std::minmax_element(v.begin(), v.end());
		EXPECT_EQ(*mn, 1);
		EXPECT_EQ(*mx, 4);
	}

	TEST(Algorithms, LexicographicalCompare)
	{
		// lexicographical_compare: true if first sequence compares less than second (dictionary order).
		std::vector<int> a{ 1, 2, 2 }, b{ 1, 2, 3 };
		EXPECT_TRUE(std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()));
		EXPECT_FALSE(std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end()));
	}

	TEST(Algorithms, LexicographicalCompareThreeWay)
	{
		// lexicographical_compare_three_way (C++20): three-way ordering of two ranges (strong/partial/weak ordering).
		std::vector<int> a{ 1, 2 }, b{ 1, 2, 0 };
		auto ord = std::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end());
		EXPECT_EQ(ord, std::strong_ordering::less);
	}

	TEST(Algorithms, NextPermutation)
	{
		// next_permutation: transforms to lexicographically next permutation; returns false if already last permutation.
		std::vector<int> v{ 1, 2, 3 };
		EXPECT_TRUE(std::next_permutation(v.begin(), v.end()));
		EXPECT_EQ(v, (std::vector<int>{1, 3, 2}));
	}

	TEST(Algorithms, PrevPermutation)
	{
		// prev_permutation: transforms to lexicographically previous permutation.
		std::vector<int> v{ 3, 2, 1 };
		EXPECT_TRUE(std::prev_permutation(v.begin(), v.end()));
		EXPECT_EQ(v, (std::vector<int>{3, 1, 2}));
	}

	TEST(Algorithms, Clamp)
	{
		// clamp (C++17): returns v projected into [lo, hi] (expects lo <= hi).
		EXPECT_EQ(std::clamp(5, 0, 10), 5);
		EXPECT_EQ(std::clamp(-1, 0, 10), 0);
		EXPECT_EQ(std::clamp(99, 0, 10), 10);
	}

	TEST(Algorithms, Min)
	{
		// min (initializer_list): smallest value in the braced list (also overloads for two values / custom compare).
		EXPECT_EQ(std::min({3, 1, 4, 1, 5}), 1);
	}

	TEST(Algorithms, Max)
	{
		// max (initializer_list): largest value in the braced list.
		EXPECT_EQ(std::max({3, 1, 4, 1, 5}), 5);
	}

	TEST(Algorithms, Minmax)
	{
		// minmax (initializer_list): pair (min, max) from the list in one pass.
		auto [mn, mx] = std::minmax({3, 1, 4});
		EXPECT_EQ(mn, 1);
		EXPECT_EQ(mx, 4);
	}

} // namespace
