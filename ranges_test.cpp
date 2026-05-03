#include <gtest/gtest.h>

#include <array>
#include <compare>
#include <iterator>
#include <list>
#include <numeric>
#include <ranges>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>


namespace {

	// ---------------------------------------------------------------------------
	// Range access customization point objects (std::ranges::begin, end, ...)
	// These dispatch to member/non-member hooks so generic range algorithms work
	// on C arrays, containers, and custom types.
	// ---------------------------------------------------------------------------

	TEST(Ranges, Begin)
	{
		// Verifies that ranges::begin obtains an iterator to the first element of
		// an lvalue range, matching the library’s generic “start of sequence” hook.
		std::vector<int> v{ 10, 20, 30 };
		EXPECT_EQ(*std::ranges::begin(v), 10);
	}


	TEST(Ranges, End)
	{
		// Verifies that ranges::end pairs with begin: iterating [begin, end) covers
		// exactly the elements of the range without reading past the last element.
		std::vector<int> v{ 1, 2, 3 };
		EXPECT_EQ(std::ranges::end(v) - std::ranges::begin(v), 3);
	}

	TEST(Ranges, Cbegin)
	{
		// cbegin forces const traversal even on a non-const container, which is
		// how const-correct algorithms obtain read-only iterators.
		std::vector<int> v{ 7 };
		EXPECT_EQ(*std::ranges::cbegin(v), 7);
		static_assert(std::is_same_v<decltype(*std::ranges::cbegin(v)), const int&>);
	}

	TEST(Ranges, Cend)
	{
		// cend is the const sentinel matching cbegin; together they delimit the
		// sequence as seen through const access.
		std::vector<int> v{ 1, 2 };
		EXPECT_EQ(std::ranges::cend(v) - std::ranges::cbegin(v), 2);
	}

	TEST(Ranges, Rbegin)
	{
		// rbegin exposes reverse iteration starting at the last element; this is
		// the hook used by reverse_view and reverse adapters.
		std::vector<int> v{ 1, 2, 3 };
		EXPECT_EQ(*std::ranges::rbegin(v), 3);
	}

	TEST(Ranges, Rend)
	{
		// rend is the reverse sentinel one past the “reverse front”; the half-open
		// interval [rbegin, rend) visits elements from back to front.
		std::vector<int> v{ 1, 2, 3 };
		EXPECT_EQ(std::ranges::rend(v) - std::ranges::rbegin(v), 3);
	}

	TEST(Ranges, Crbegin)
	{
		// crbegin is the const version of rbegin, so dereferenced elements are
		// immutable even if the underlying container is non-const.
		std::vector<int> v{ 5 };
		EXPECT_EQ(*std::ranges::crbegin(v), 5);
	}

	TEST(Ranges, Crend)
	{
		// crend closes the const reverse range [crbegin, crend).
		std::vector<int> v{ 1, 2 };
		EXPECT_EQ(std::ranges::crend(v) - std::ranges::crbegin(v), 2);
	}

	TEST(Ranges, Size)
	{
		// ranges::size reports the number of elements for sized ranges (containers,
		// arrays, etc.) in a uniform way for generic code.
		std::vector<int> v(4);
		EXPECT_EQ(std::ranges::size(v), 4u);
	}

	TEST(Ranges, Ssize)
	{
		// ssize is the signed counterpart to size, avoiding unsigned underflow
		// bugs when subtracting or comparing against zero in numeric code.
		std::vector<int> v{ 1, 2, 3 };
		EXPECT_EQ(std::ranges::ssize(v), 3);
	}

	TEST(Ranges, Empty)
	{
		// empty is the semantic “has no elements” check; for forward+ ranges it is
		// equivalent to begin == end but can be more efficient for known-sized types.
		std::vector<int> a, b{ 1 };
		EXPECT_TRUE(std::ranges::empty(a));
		EXPECT_FALSE(std::ranges::empty(b));
	}

	TEST(Ranges, Data)
	{
		// data exposes contiguous storage for contiguous ranges (e.g. vector),
		// enabling C APIs and pointer arithmetic on the underlying buffer.
		std::vector<int> v{ 9, 8 };
		EXPECT_EQ(std::ranges::data(v)[0], 9);
	}

	TEST(Ranges, Cdata)
	{
		// cdata is the const-qualified contiguous pointer; use it when the
		// sequence must not be mutated through that pointer.
		std::vector<int> v{ 3 };
		EXPECT_EQ(std::ranges::cdata(v)[0], 3);
		static_assert(std::is_same_v<decltype(std::ranges::cdata(v)), const int*>);
	}

	// ---------------------------------------------------------------------------
	// Range-associated swaps (std::ranges::swap, std::ranges::iter_swap)
	// ---------------------------------------------------------------------------

	TEST(Ranges, Swap)
	{
		// ranges::swap is the customization-point aware swap used throughout the
		// ranges design; it finds the correct swap via ADL or falls back to std.
		int x = 1, y = 2;
		std::ranges::swap(x, y);
		EXPECT_EQ(x, 2);
		EXPECT_EQ(y, 1);
	}

	TEST(Ranges, IterSwap)
	{
		// iter_swap exchanges the values referred to by two iterators without
		// requiring the iterators to be random-access or the types to be assignable
		// in other patterns; views rely on this for many iterator operations.
		std::vector<int> v{ 1, 2 };
		std::ranges::iter_swap(v.begin(), v.begin() + 1);
		EXPECT_EQ(v[0], 2);
		EXPECT_EQ(v[1], 1);
	}

	// ---------------------------------------------------------------------------
	// Alias templates (iterator_t, sentinel_t, range_*_t)
	// ---------------------------------------------------------------------------

	TEST(Ranges, IteratorT)
	{
		// iterator_t<R> is decltype(begin(r)) for an lvalue R; it is the type every
		// range algorithm uses when naming “the iterator type of this range”.
		static_assert(std::same_as<std::ranges::iterator_t<std::vector<int>&>, std::vector<int>::iterator>);
		SUCCEED();
	}

	TEST(Ranges, SentinelT)
	{
		// sentinel_t<R> is decltype(end(r)); for most containers it equals
		// iterator_t, but for sentinel-based ranges it can differ (e.g. null-terminated C strings).
		static_assert(std::same_as<std::ranges::sentinel_t<std::vector<int>&>, std::vector<int>::iterator>);
		SUCCEED();
	}

	TEST(Ranges, RangeSizeT)
	{
		// range_size_t is the type of ranges::size(r) for sized_range R, usually
		// size_t but kept abstract so generic code names the right width.
		static_assert(std::same_as<std::ranges::range_size_t<std::vector<int>>, std::size_t>);
		SUCCEED();
	}

	TEST(Ranges, RangeDifferenceT)
	{
		// range_difference_t is the iterator difference type for the range’s
		// begin iterator—what you get when subtracting two positions.
		static_assert(std::same_as<std::ranges::range_difference_t<std::vector<int>>, std::ptrdiff_t>);
		SUCCEED();
	}

	TEST(Ranges, RangeValueT)
	{
		// range_value_t strips references from the iterator’s value_type; it is
		// the “element type” of the range for algorithms like copy or transform.
		static_assert(std::same_as<std::ranges::range_value_t<std::vector<int>>, int>);
		SUCCEED();
	}

	TEST(Ranges, RangeReferenceT)
	{
		// range_reference_t is what operator* on the iterator returns—often T& for
		// mutable traversal or const T& for const ranges.
		static_assert(std::same_as<std::ranges::range_reference_t<std::vector<int>>, int&>);
		SUCCEED();
	}

	TEST(Ranges, RangeRvalueReferenceT)
	{
		// range_rvalue_reference_t is the rvalue reference type produced when
		// moving through the range (e.g. iter_move), important for move-only values.
		static_assert(std::same_as<std::ranges::range_rvalue_reference_t<std::vector<int>>, int&&>);
		SUCCEED();
	}

	TEST(Ranges, RangeCommonReferenceT)
	{
		// range_common_reference_t is the common reference type used when mixing
		// references in proxy iterators or zip-like patterns.
		static_assert(
			std::same_as<std::ranges::range_common_reference_t<std::vector<int>>, int&>);
		SUCCEED();
	}

	// ---------------------------------------------------------------------------
	// Core range concepts (compile-time requirements used by all range APIs)
	// ---------------------------------------------------------------------------

	TEST(Ranges, ConceptRange)
	{
		// range<T> requires that begin and end produce an iterator and a sentinel;
		// it is the broadest “is a sequence” concept in the library.
		static_assert(std::ranges::range<std::vector<int>>);
		SUCCEED();
	}

	TEST(Ranges, ConceptBorrowedRange)
	{
		// borrowed_range means iterators from the range do not dangle when the
		// range object is destroyed (e.g. lvalue containers, string_view, subrange).
		static_assert(std::ranges::borrowed_range<std::vector<int>&>);
		static_assert(!std::ranges::borrowed_range<std::vector<int>>);
		SUCCEED();
	}

	TEST(Ranges, ConceptSizedRange)
	{
		// sized_range guarantees O(1) size via ranges::size; algorithms can
		// pre-allocate or bound work using that information.
		static_assert(std::ranges::sized_range<std::vector<int>>);
		SUCCEED();
	}

	TEST(Ranges, ConceptView)
	{
		// view is a cheap-to-copy range (typically O(1) copy); views compose without
		// owning storage and are the building blocks of the lazy pipeline API.
		static_assert(std::ranges::view<std::ranges::empty_view<int>>);
		SUCCEED();
	}

	TEST(Ranges, ConceptInputRange)
	{
		// input_range refines range with input_iterator: single-pass read.
		static_assert(!std::ranges::input_range<std::istringstream&>);
		static_assert(std::ranges::input_range<std::vector<int>>);
		SUCCEED();
	}

	TEST(Ranges, ConceptForwardRange)
	{
		// forward_range allows multi-pass iteration with stable object identity.
		static_assert(std::ranges::forward_range<std::vector<int>>);
		SUCCEED();
	}

	TEST(Ranges, ConceptBidirectionalRange)
	{
		// bidirectional_range adds decrement; reverse_view requires this on its base.
		static_assert(std::ranges::bidirectional_range<std::list<int>>);
		SUCCEED();
	}

	TEST(Ranges, ConceptRandomAccessRange)
	{
		// random_access_range adds constant-time advance and subscripting.
		static_assert(std::ranges::random_access_range<std::vector<int>>);
		SUCCEED();
	}

	TEST(Ranges, ConceptContiguousRange)
	{
		// contiguous_range promises elements in contiguous memory with data()+i.
		static_assert(std::ranges::contiguous_range<std::vector<int>>);
		SUCCEED();
	}

	TEST(Ranges, ConceptCommonRange)
	{
		// common_range means iterator_t and sentinel_t are the same type, which
		// simplifies some legacy interfaces and common_view’s purpose.
		static_assert(std::ranges::common_range<std::vector<int>>);
		SUCCEED();
	}

	TEST(Ranges, ConceptViewableRange)
	{
		// viewable_range is what may be passed to views::all: either a view or an
		// lvalue non-view range that can be wrapped (e.g. ref_view).
		std::vector<int> v;
		static_assert(std::ranges::viewable_range<decltype((v))>);
		SUCCEED();
	}

	TEST(Ranges, ConceptOutputRange)
	{
		// output_range<R, T> means iterators into R can write values of type T.
		static_assert(std::ranges::output_range<std::vector<int>, int>);
		SUCCEED();
	}

	// ---------------------------------------------------------------------------
	// enable_borrowed_range / enable_view / view_base / view_interface
	// ---------------------------------------------------------------------------

	TEST(Ranges, EnableBorrowedRange)
	{
		// Library types specialize enable_borrowed_range to true when iterators
		// remain valid independent of the range object’s lifetime (e.g. subrange).
		static_assert(std::ranges::enable_borrowed_range<std::ranges::subrange<std::vector<int>::iterator,
			std::vector<int>::iterator>>);
		SUCCEED();
	}

	TEST(Ranges, EnableView)
	{
		// enable_view is the opt-in trait for types that should model view; empty_view
		// and other views specialize this so copying stays shallow/O(1).
		static_assert(std::ranges::enable_view<std::ranges::empty_view<int>>);
		SUCCEED();
	}

	TEST(Ranges, ViewBase)
	{
		// view_base is an empty CRTP tag; inheriting it marks a type as intending
		// to be a view for enable_view’s default logic.
		struct Tag : std::ranges::view_base
		{
		};
		static_assert(std::is_empty_v<Tag>);
		SUCCEED();
	}

	TEST(Ranges, ViewInterface)
	{
		// view_interface<D> supplies default empty/size/data members when D exposes
		// only begin/end, reducing boilerplate for user-defined views.
		struct IntSpan : std::ranges::view_interface<IntSpan>
		{
			std::array<int, 2> a{ 4, 5 };
			auto begin() const { return a.begin(); }
			auto end() const { return a.end(); }
		};
		IntSpan s;
		EXPECT_EQ(std::ranges::size(s), 2u);
		static_assert(std::ranges::view<IntSpan>);
	}

	// ---------------------------------------------------------------------------
	// subrange_kind, subrange, tuple-like access via std::ranges::get
	// ---------------------------------------------------------------------------

	TEST(Ranges, SubrangeKind)
	{
		// subrange_kind selects whether a subrange stores only iterators or also
		// caches size for O(1) ranges::size when the sentinel isn’t a sized match.
		using K = std::ranges::subrange_kind;
		EXPECT_EQ(static_cast<bool>(K::unsized), false);
		EXPECT_EQ(static_cast<bool>(K::sized), true);
	}

	TEST(Ranges, Subrange)
	{
		// subrange bundles iterator+sentinel into a borrowed view, bridging raw
		// iterator pairs into the range/view machinery.
		std::vector<int> v{ 1, 2, 3, 4 };
		std::ranges::subrange sub{ v.begin() + 1, v.end() };
		EXPECT_EQ(std::ranges::size(sub), 3u);
		EXPECT_EQ(*std::ranges::begin(sub), 2);
	}

	TEST(Ranges, GetSubrange)
	{
		// std::ranges::get<0/1> on subrange exposes iterator and sentinel for
		// structured bindings and tuple-like decomposition.
		std::vector<int> v{ 1, 2, 3 };
		std::ranges::subrange s{ v.begin(), v.end() };
		EXPECT_EQ(std::get<0>(s), v.begin());
		EXPECT_EQ(std::get<1>(s), v.end());
	}

	// ---------------------------------------------------------------------------
	// dangling, borrowed_iterator_t, borrowed_subrange_t
	// ---------------------------------------------------------------------------

	TEST(Ranges, Dangling)
	{
		// dangling is a sentinel type returned instead of an iterator when the
		// algorithm would otherwise hand back an iterator into a destroyed temp.
		std::ranges::dangling d{};
		(void)d;
		SUCCEED();
	}

	TEST(Ranges, BorrowedIteratorT)
	{
		// For an lvalue borrowed range, borrowed_iterator_t is the real iterator;
		// for a non-borrowed prvalue it decays to dangling so misuse fails to type-check.
		static_assert(
			std::same_as<std::ranges::borrowed_iterator_t<std::vector<int>&>, std::vector<int>::iterator>);
		static_assert(std::same_as<std::ranges::borrowed_iterator_t<std::vector<int>>, std::ranges::dangling>);
		SUCCEED();
	}

	TEST(Ranges, BorrowedSubrangeT)
	{
		// borrowed_subrange_t is the subrange type over borrowed iterators, or
		// dangling when the input range cannot be safely borrowed.
		static_assert(std::same_as<std::ranges::borrowed_subrange_t<std::vector<int>&>,
			std::ranges::subrange<std::vector<int>::iterator, std::vector<int>::iterator>>);
		SUCCEED();
	}

	// ---------------------------------------------------------------------------
	// Namespace alias: std::views == std::ranges::views
	// ---------------------------------------------------------------------------

	TEST(Ranges, NamespaceViewsAlias)
	{
		// The standard provides std::views as an alias for std::ranges::views so
		// pipeline code can spell std::views::filter without the extra ranges::.
		std::vector<int> v{ 1, 2, 3, 4 };
		auto a = v | std::views::filter([](int x) { return x % 2 == 0; });
		auto b = v | std::ranges::views::filter([](int x) { return x % 2 == 0; });
		EXPECT_EQ(std::ranges::distance(a), std::ranges::distance(b));
	}

	// ---------------------------------------------------------------------------
	// Views::all, all_t, ref, ref_view
	// ---------------------------------------------------------------------------

	TEST(Ranges, ViewsAll)
	{
		// views::all turns a viewable_range into a view: views pass through,
		// lvalue containers become ref_view wrappers referencing existing storage.
		std::vector<int> v{ 1, 2, 3 };
		auto w = std::views::all(v);
		EXPECT_EQ(std::ranges::size(w), 3u);
	}

	TEST(Ranges, ViewsAllT)
	{
		// all_t<R> is decltype(all(declval<R>())), naming the concrete view type
		// produced when a particular range is adapted with views::all.
		using T = std::views::all_t<std::vector<int>&>;
		static_assert(std::same_as<T, std::ranges::ref_view<std::vector<int>>>);
		SUCCEED();
	}

	TEST(Ranges, RefView)
	{
		// ref_view stores a pointer to an external range’s storage; it is non-owning
		// and makes lvalue containers usable inside view pipelines.
		std::vector<int> v{ 8, 9 };
		std::ranges::ref_view rv{ v };
		EXPECT_EQ(std::ranges::size(rv), 2u);
	}

	TEST(Ranges, ViewsRef)
	{
		// views::ref is the range adaptor object that wraps an lvalue in ref_view;
		// on implementations without views::ref, views::all on an lvalue is specified
		// to produce the same ref_view type and non-owning semantics.
		std::vector<int> v{ 1 };
		auto r = std::views::all(v);
		static_assert(std::same_as<decltype(r), std::ranges::ref_view<std::vector<int>>>);
		EXPECT_EQ(*std::ranges::begin(r), 1);
	}

	// ---------------------------------------------------------------------------
	// empty_view, views::empty
	// ---------------------------------------------------------------------------

	TEST(Ranges, EmptyView)
	{
		// empty_view<T> is a zero-length view with a chosen value_type; it is the
		// identity for some algebraic properties of views and a useful default.
		std::ranges::empty_view<int> ev;
		EXPECT_TRUE(std::ranges::empty(ev));
		static_assert(std::ranges::view<decltype(ev)>);
	}

	TEST(Ranges, ViewsEmpty)
	{
		// views::empty<T> is the canonical empty_view<T> object (variable template).
		auto& e = std::views::empty<int>;
		EXPECT_EQ(std::ranges::size(e), 0u);
	}

	// ---------------------------------------------------------------------------
	// single_view, views::single
	// ---------------------------------------------------------------------------

	TEST(Ranges, SingleView)
	{
		// single_view holds exactly one element lazily; useful for joining or
		// padding pipelines without allocating a container.
		std::ranges::single_view sv{ 42 };
		EXPECT_EQ(*std::ranges::begin(sv), 42);
		EXPECT_EQ(std::ranges::size(sv), 1u);
	}

	TEST(Ranges, ViewsSingle)
	{
		// views::single(x) builds a single_view over x.
		auto s = std::views::single(std::string{ "hi" });
		EXPECT_EQ(*std::ranges::begin(s), std::string{ "hi" });
	}

	// ---------------------------------------------------------------------------
	// iota_view, views::iota
	// ---------------------------------------------------------------------------

	TEST(Ranges, IotaView)
	{
		// iota_view generates a lazy arithmetic sequence from a weakly_incrementable
		// start, optionally bounded, without materializing all values.
		std::ranges::iota_view iv{ 0, 3 };
		EXPECT_EQ(std::ranges::distance(iv), 3);
	}

	TEST(Ranges, ViewsIota)
	{
		// views::iota is the range factory for unbounded or bounded iota sequences.
		auto seq = std::views::iota(10, 13);
		std::vector<int> out;
		std::ranges::copy(seq, std::back_inserter(out));
		EXPECT_EQ(out, (std::vector<int>{10, 11, 12}));
	}

	// ---------------------------------------------------------------------------
	// basic_istream_view, istream_view, wistream_view, views::istream
	// ---------------------------------------------------------------------------

	TEST(Ranges, BasicIstreamView)
	{
		// basic_istream_view reads successive values from a stream via operator>>;
		// it is a single-pass input view over formatted extraction.
		std::istringstream iss("1 2 3");
		std::ranges::basic_istream_view<int, char> bv{ iss };
		auto it = std::ranges::begin(bv);
		EXPECT_EQ(*it, 1);
	}

	TEST(Ranges, IstreamView)
	{
		// istream_view is the char narrow-stream alias of basic_istream_view.
		std::istringstream iss("7");
		std::ranges::istream_view<int> iv{ iss };
		EXPECT_EQ(*std::ranges::begin(iv), 7);
	}

	TEST(Ranges, WIstreamView)
	{
		// wistream_view is the wchar_t alias for wide stream extraction.
		std::wistringstream wiss(L"99");
		std::ranges::wistream_view<int> wiv{ wiss };
		EXPECT_EQ(*std::ranges::begin(wiv), 99);
	}

	TEST(Ranges, ViewsIstream)
	{
		// views::istream<T>(stream) constructs the istream_view with a clearer name.
		std::istringstream iss("5");
		auto v = std::views::istream<int>(iss);
		EXPECT_EQ(*std::ranges::begin(v), 5);
	}

	// ---------------------------------------------------------------------------
	// filter_view, views::filter
	// ---------------------------------------------------------------------------

	TEST(Ranges, FilterView)
	{
		// filter_view lazily skips elements that fail a predicate while preserving
		// the base order; it may not satisfy sized_range even if the base does.
		std::vector<int> v{ 1, 2, 3, 4 };
		std::ranges::filter_view fv{ v, [](int x) { return x % 2 == 0; } };
		EXPECT_EQ(std::ranges::distance(fv), 2);
	}

	TEST(Ranges, ViewsFilter)
	{
		// views::filter(pred) is the pipeable adaptor closure for filter_view.
		std::vector<int> v{ 1, 2, 3 };
		auto w = v | std::views::filter([](int x) { return x >= 2; });
		EXPECT_EQ(std::ranges::distance(w), 2);
	}

	// ---------------------------------------------------------------------------
	// transform_view, views::transform
	// ---------------------------------------------------------------------------

	TEST(Ranges, TransformView)
	{
		// transform_view applies a function to each element on demand, producing a
		// new read-only projection without storing transformed values.
		std::vector<int> v{ 1, 2, 3 };
		std::ranges::transform_view tv{ v, [](int x) { return x * x; } };
		EXPECT_EQ(*std::ranges::begin(tv), 1);
	}

	TEST(Ranges, ViewsTransform)
	{
		// views::transform(f) pipes a mapping over any input viewable range.
		std::vector<int> v{ 2, 3 };
		auto w = v | std::views::transform([](int x) { return x + 1; });
		EXPECT_EQ(std::ranges::distance(w), 2);
		EXPECT_EQ(*std::ranges::begin(w), 3);
	}

	// ---------------------------------------------------------------------------
	// take_view, views::take
	// ---------------------------------------------------------------------------

	TEST(Ranges, TakeView)
	{
		// take_view truncates the base to at most N elements while keeping lazy
		// evaluation for the remainder of the base.
		std::vector<int> v{ 1, 2, 3, 4, 5 };
		std::ranges::take_view tv{ v, 3u };
		EXPECT_EQ(std::ranges::size(tv), 3u);
	}

	TEST(Ranges, ViewsTake)
	{
		// views::take(N) is the pipe adaptor limiting length.
		std::vector<int> v{ 1, 2, 3, 4 };
		auto w = v | std::views::take(2);
		EXPECT_EQ(std::ranges::distance(w), 2);
	}

	// ---------------------------------------------------------------------------
	// take_while_view, views::take_while
	// ---------------------------------------------------------------------------

	TEST(Ranges, TakeWhileView)
	{
		// take_while yields the longest prefix where the predicate holds; the first
		// false ends the view even if later elements would satisfy it.
		std::vector<int> v{ 1, 2, 10, 3 };
		std::ranges::take_while_view tw{ v, [](int x) { return x < 5; } };
		EXPECT_EQ(std::ranges::distance(tw), 2);
	}

	TEST(Ranges, ViewsTakeWhile)
	{
		// views::take_while(pred) composes the same behavior in pipeline form.
		std::vector<int> v{ 1, 2, 3 };
		auto w = v | std::views::take_while([](int x) { return x < 3; });
		EXPECT_EQ(std::ranges::distance(w), 2);
	}

	// ---------------------------------------------------------------------------
	// drop_view, views::drop
	// ---------------------------------------------------------------------------

	TEST(Ranges, DropView)
	{
		// drop_view skips the first N elements of the base; remaining elements are
		// traversed lazily with the same properties as the base (except size).
		std::vector<int> v{ 1, 2, 3, 4 };
		std::ranges::drop_view dv{ v, 2u };
		EXPECT_EQ(*std::ranges::begin(dv), 3);
	}

	TEST(Ranges, ViewsDrop)
	{
		// views::drop(N) is the pipe spelling for skipping a fixed prefix.
		std::vector<int> v{ 9, 8, 7 };
		auto w = v | std::views::drop(1);
		EXPECT_EQ(*std::ranges::begin(w), 8);
	}

	// ---------------------------------------------------------------------------
	// drop_while_view, views::drop_while
	// ---------------------------------------------------------------------------

	TEST(Ranges, DropWhileView)
	{
		// drop_while removes the longest initial subsequence satisfying the
		// predicate, then exposes the rest of the base unchanged.
		std::vector<int> v{ 1, 2, 3, 4 };
		std::ranges::drop_while_view dw{ v, [](int x) { return x < 3; } };
		EXPECT_EQ(*std::ranges::begin(dw), 3);
	}

	TEST(Ranges, ViewsDropWhile)
	{
		// views::drop_while(pred) is the adaptor form used in pipelines.
		std::vector<int> v{ 0, 0, 5 };
		auto w = v | std::views::drop_while([](int x) { return x == 0; });
		EXPECT_EQ(*std::ranges::begin(w), 5);
	}

	// ---------------------------------------------------------------------------
	// join_view, views::join
	// ---------------------------------------------------------------------------

	TEST(Ranges, JoinView)
	{
		// join_view flattens a range-of-ranges into a single linear sequence,
		// visiting inner elements in order without allocating a merged container.
		std::vector<std::vector<int>> outer{ {1, 2}, {3} };
		std::ranges::join_view jv{ outer };
		EXPECT_EQ(std::ranges::distance(jv), 3);
	}

	TEST(Ranges, ViewsJoin)
	{
		// views::join flattens in pipe form; often used after split-like views.
		std::vector<std::vector<int>> outer{ {10}, {20, 30} };
		auto w = outer | std::views::join;
		std::vector<int> flat;
		std::ranges::copy(w, std::back_inserter(flat));
		EXPECT_EQ(flat, (std::vector<int>{10, 20, 30}));
	}

	// ---------------------------------------------------------------------------
	// split_view, lazy_split_view, views::split, views::lazy_split
	// ---------------------------------------------------------------------------

	TEST(Ranges, SplitView)
	{
		// split_view requires forward ranges and splits on a delimiter pattern,
		// producing outer iterators over inner subranges between matches.
		std::string s{ "a,b,c" };
		std::ranges::split_view sv{ s, std::string{","} };
		auto outer = std::ranges::begin(sv);
		std::string first_chunk{ std::ranges::begin(*outer), std::ranges::end(*outer) };
		EXPECT_EQ(first_chunk, "a");
	}

	TEST(Ranges, LazySplitView)
	{
		// lazy_split_view can split input ranges with less requirement on the base
		// in some cases and evaluates split boundaries lazily as you advance.
		std::string s{ "x|y" };
		std::ranges::lazy_split_view lsv{ s, std::string{"|"} };
		auto it = std::ranges::begin(lsv);
		std::string chunk;
		for (char c : *it)
			chunk += c;
		EXPECT_EQ(chunk, "x");
	}

	TEST(Ranges, ViewsSplit)
	{
		// views::split(delimiter) is the usual string/tokenization adaptor.
		std::string s{ "one.two" };
		auto parts = s | std::views::split('.');
		auto it = std::ranges::begin(parts);
		std::string first{ std::ranges::begin(*it), std::ranges::end(*it) };
		EXPECT_EQ(first, "one");
	}

	TEST(Ranges, ViewsLazySplit)
	{
		// views::lazy_split is the lazy counterpart for delimiter-based splitting.
		std::string s{ "p-q" };
		auto parts = s | std::views::lazy_split('-');
		auto it = std::ranges::begin(parts);
		std::string first;
		for (char c : *it)
			first += c;
		EXPECT_EQ(first, "p");
	}

	// ---------------------------------------------------------------------------
	// common_view, views::common
	// ---------------------------------------------------------------------------

	TEST(Ranges, CommonView)
	{
		// common_view adapts a range whose iterator and sentinel types differ into
		// one whose end is the same type as begin, for APIs that need common_type.
		std::vector<int> v{ 1, 2, 3, 4, 5 };
		auto tw = v | std::views::take_while([](int x) { return x < 4; });
		static_assert(!std::ranges::common_range<decltype(tw)>);
		std::ranges::common_view cv{ tw };
		static_assert(std::ranges::common_range<decltype(cv)>);
		EXPECT_EQ(std::ranges::distance(cv), 3);
	}

	TEST(Ranges, ViewsCommon)
	{
		// views::common is the pipe adaptor producing common_view from a non-common base.
		std::vector<int> v{ 1, 2, 3, 4 };
		auto tw = v | std::views::take_while([](int x) { return x < 4; });
		auto w = tw | std::views::common;
		EXPECT_EQ(std::ranges::distance(w), 3);
	}

	// ---------------------------------------------------------------------------
	// reverse_view, views::reverse
	// ---------------------------------------------------------------------------

	TEST(Ranges, ReverseView)
	{
		// reverse_view presents the base bidirectional range in reverse order
		// without copying elements.
		std::vector<int> v{ 1, 2, 3 };
		std::ranges::reverse_view rv{ v };
		EXPECT_EQ(*std::ranges::begin(rv), 3);
	}

	TEST(Ranges, ViewsReverse)
	{
		// views::reverse adapts any bidirectional viewable range in pipeline style.
		std::vector<int> v{ 1, 2, 3 };
		auto w = v | std::views::reverse;
		EXPECT_EQ(std::ranges::distance(w), 3);
		EXPECT_EQ(*std::ranges::begin(w), 3);
	}

	// ---------------------------------------------------------------------------
	// elements_view, views::elements
	// ---------------------------------------------------------------------------

	TEST(Ranges, ElementsView)
	{
		// elements_view<N> projects the Nth element from each tuple-like value in
		// the base range (e.g. struct-of-fields or pair components).
		std::vector<std::tuple<int, char>> v{ {1, 'a'}, {2, 'b'} };
		auto base = std::views::all(v);
		std::ranges::elements_view<decltype(base), 0> ev{ base };
		EXPECT_EQ(*std::ranges::begin(ev), 1);
	}

	TEST(Ranges, ViewsElements)
	{
		// views::elements<N> is the pipeable projection for the Nth tuple element.
		std::vector<std::pair<int, int>> v{ {1, 10}, {2, 20} };
		auto w = v | std::views::elements<0>;
		std::vector<int> out;
		std::ranges::copy(w, std::back_inserter(out));
		EXPECT_EQ(out, (std::vector<int>{1, 2}));
	}

	// ---------------------------------------------------------------------------
	// views::keys, views::values (pair-like projection adaptors)
	// ---------------------------------------------------------------------------

	TEST(Ranges, ViewsKeys)
	{
		// views::keys is shorthand for elements<0> on pair-like ranges (maps, zip).
		std::vector<std::pair<int, int>> v{ {3, 30}, {4, 40} };
		auto w = v | std::views::keys;
		EXPECT_EQ(*std::ranges::begin(w), 3);
	}

	TEST(Ranges, ViewsValues)
	{
		// views::values projects the second component of each pair-like element.
		std::vector<std::pair<int, int>> v{ {1, 100} };
		auto w = v | std::views::values;
		EXPECT_EQ(*std::ranges::begin(w), 100);
	}

	// ---------------------------------------------------------------------------
	// views::counted (iterator + count subrange as a view)
	// ---------------------------------------------------------------------------

	TEST(Ranges, ViewsCounted)
	{
		// views::counted(it, n) presents exactly n elements starting at it without
		// needing a separate end iterator; useful with raw pointers or algorithms.
		std::vector<int> v{ 10, 20, 30, 40 };
		auto w = std::views::counted(v.begin() + 1, 2);
		EXPECT_EQ(std::ranges::distance(w), 2);
		EXPECT_EQ(*std::ranges::begin(w), 20);
	}

	// ---------------------------------------------------------------------------
	// owning_view (C++20): move-only owning wrapper for rvalue containers
	// ---------------------------------------------------------------------------

	TEST(Ranges, OwningView)
	{
		// owning_view takes ownership of an rvalue range so it can participate in
		// view pipelines while keeping storage alive inside the view object.
		std::ranges::owning_view ov{ std::vector<int>{1, 2, 3} };
		EXPECT_EQ(std::ranges::size(ov), 3u);
	}

} // namespace
