#include <gtest/gtest.h>

#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace {

	struct MoveProbe
	{
		int value = 0;
		explicit MoveProbe(int v) : value(v) {}
		MoveProbe(MoveProbe&& other) noexcept : value(other.value) { other.value = -1; }
		MoveProbe& operator=(MoveProbe&& other) noexcept
		{
			value = other.value;
			other.value = -1;
			return *this;
		}
		MoveProbe(const MoveProbe&) = delete;
		MoveProbe& operator=(const MoveProbe&) = delete;
	};

	template <std::size_t... Is>
	constexpr std::size_t SumIndices(std::index_sequence<Is...>)
	{
		return (Is + ... + 0u);
	}

	TEST(Utility, PairConstructionAssignmentComparisonAndSwap)
	{
		// std::pair is the fundamental 2-tuple utility type from <utility>, with
		// value semantics, lexicographic comparisons, and swap support.
		std::pair<int, std::string> a{ 1, "one" };
		std::pair<int, std::string> b{ 2, "two" };

		EXPECT_EQ(a.first, 1);
		EXPECT_EQ(a.second, "one");
		EXPECT_TRUE(a < b);
		EXPECT_TRUE(a != b);

		a.swap(b);
		EXPECT_EQ(a.first, 2);
		EXPECT_EQ(a.second, "two");

		using std::swap;
		swap(a, b);
		EXPECT_EQ(a.first, 1);
		EXPECT_EQ(a.second, "one");
	}

	TEST(Utility, MakePairAndPiecewiseConstruction)
	{
		// make_pair deduces pair element types; piecewise_construct enables tuple-
		// based constructor forwarding into pair members.
		auto p = std::make_pair(7, std::string("seven"));
		EXPECT_TRUE((std::is_same_v<decltype(p), std::pair<int, std::string>>));
		EXPECT_EQ(p.first, 7);
		EXPECT_EQ(p.second, "seven");

		std::pair<std::string, std::string> piecewise(
			std::piecewise_construct,
			std::forward_as_tuple(3, 'a'),
			std::forward_as_tuple("bbb"));
		EXPECT_EQ(piecewise.first, "aaa");
		EXPECT_EQ(piecewise.second, "bbb");
	}

	TEST(Utility, MoveForwardAndForwardLikePreserveValueCategories)
	{
		// move converts to rvalue reference; forward preserves value category based
		// on template argument; forward_like applies cvref qualifiers of a model.
		int x = 42;
		EXPECT_TRUE((std::is_same_v<decltype(std::move(x)), int&&>));
		EXPECT_TRUE((std::is_same_v<decltype(std::forward<int&>(x)), int&>));
		EXPECT_TRUE((std::is_same_v<decltype(std::forward<int>(x)), int&&>));

#ifdef __cpp_lib_forward_like
		EXPECT_GE(__cpp_lib_forward_like, 202207L);
		const int model = 0;
		const int y = 7;
		EXPECT_TRUE((std::is_same_v<decltype(std::forward_like<const int&>(y)), const int&>));
		EXPECT_EQ(std::forward_like<const int&>(y), 7);
		(void)model;
#else
		GTEST_SKIP() << "std::forward_like is not available.";
#endif
	}

	TEST(Utility, MoveIfNoexceptAndDeclvalTypeUtilities)
	{
		// move_if_noexcept chooses move or const lvalue based on noexcept move
		// constructibility and copyability. declval enables unevaluated type queries.
		MoveProbe probe(9);
		auto&& moved = std::move_if_noexcept(probe);
		EXPECT_TRUE((std::is_same_v<decltype(moved), MoveProbe&&>));

		EXPECT_TRUE((std::is_same_v<decltype(std::declval<int>()), int&&>));
	}

	TEST(Utility, ExchangeAndAsConstHelpers)
	{
		// exchange replaces an object's value and returns the old value. as_const
		// adds const lvalue qualification without copying.
		std::string text = "before";
		const std::string old = std::exchange(text, std::string("after"));
		EXPECT_EQ(old, "before");
		EXPECT_EQ(text, "after");

		int value = 5;
		EXPECT_TRUE((std::is_same_v<decltype(std::as_const(value)), const int&>));
		EXPECT_EQ(std::as_const(value), 5);
	}

	TEST(Utility, InPlaceTagTypesAreAvailableAndDistinct)
	{
		// in_place tags coordinate in-place construction across optional/variant/any.
		EXPECT_TRUE((std::is_same_v<decltype(std::in_place), const std::in_place_t>));
		EXPECT_TRUE((std::is_same_v<decltype(std::in_place_type<int>), const std::in_place_type_t<int>>));
		EXPECT_TRUE((std::is_same_v<decltype(std::in_place_index<2>), const std::in_place_index_t<2>>));
	}

	TEST(Utility, IntegerComparisonFunctionsHandleSignedUnsignedSafely)
	{
		// cmp_equal/cmp_not_equal/cmp_less/... provide well-defined mixed signed/
		// unsigned comparisons without surprising integral promotions.
#ifdef __cpp_lib_integer_comparison_functions
		EXPECT_GE(__cpp_lib_integer_comparison_functions, 202002L);
		EXPECT_TRUE(std::cmp_equal(5, 5u));
		EXPECT_TRUE(std::cmp_not_equal(-1, 1u));
		EXPECT_TRUE(std::cmp_less(-1, 1u));
		EXPECT_TRUE(std::cmp_less_equal(3, 3u));
		EXPECT_TRUE(std::cmp_greater(10u, -1));
		EXPECT_TRUE(std::cmp_greater_equal(10u, 10));
		EXPECT_TRUE(std::in_range<unsigned>(10));
		EXPECT_FALSE(std::in_range<unsigned>(-1));
#else
		GTEST_SKIP() << "Integer comparison helpers are not available.";
#endif
	}

	TEST(Utility, IndexSequenceAndSequenceGenerationUtilities)
	{
		// integer_sequence/index_sequence families are compile-time index lists used
		// for tuple expansion and variadic metaprogramming.
		using Seq = std::index_sequence<0, 1, 2, 3>;
		using Made = std::make_index_sequence<4>;
		using ForTypePack = std::index_sequence_for<int, double, char>;

		EXPECT_TRUE((std::is_same_v<Seq, Made>));
		EXPECT_EQ(Seq::size(), 4u);
		EXPECT_EQ(ForTypePack::size(), 3u);
		EXPECT_EQ(SumIndices(Seq{}), 6u);
	}

	TEST(Utility, ToUnderlyingConvertsEnumClassValuesWhenAvailable)
	{
		// to_underlying extracts the underlying integer representation of enum class.
		enum class Mode : unsigned short { Off = 0, On = 1, Auto = 2 };
#ifdef __cpp_lib_to_underlying
		EXPECT_GE(__cpp_lib_to_underlying, 202102L);
		EXPECT_EQ(std::to_underlying(Mode::Off), 0);
		EXPECT_EQ(std::to_underlying(Mode::Auto), 2);
#else
		GTEST_SKIP() << "std::to_underlying is not available.";
#endif
	}

}  // namespace
