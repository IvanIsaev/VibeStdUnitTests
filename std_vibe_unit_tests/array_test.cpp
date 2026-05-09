#include <gtest/gtest.h>

#include <array>
#include <tuple>
#include <type_traits>

namespace {

	TEST(ArrayHeader, TypeAliasesAndCompileTimeSizeContracts)
	{
		// std::array<T, N> exposes container aliases and fixed compile-time extent.
		using A = std::array<int, 4>;
		EXPECT_TRUE((std::is_same_v<A::value_type, int>));
		EXPECT_TRUE((std::is_same_v<A::size_type, std::size_t>));
		EXPECT_TRUE((std::is_same_v<A::difference_type, std::ptrdiff_t>));
		EXPECT_TRUE((std::is_same_v<A::reference, int&>));
		EXPECT_TRUE((std::is_same_v<A::const_reference, const int&>));
		EXPECT_TRUE((std::is_same_v<A::pointer, int*>));
		EXPECT_TRUE((std::is_same_v<A::const_pointer, const int*>));

		constexpr A values{ 1, 2, 3, 4 };
		static_assert(values.size() == 4);
		EXPECT_EQ(values.size(), 4u);
	}

	TEST(ArrayHeader, ElementAccessFrontBackAtAndData)
	{
		// array provides contiguous storage with bounds-checked and unchecked access.
		std::array<int, 5> values{ 10, 20, 30, 40, 50 };

		EXPECT_EQ(values[0], 10);
		EXPECT_EQ(values.front(), 10);
		EXPECT_EQ(values.back(), 50);
		EXPECT_EQ(values.at(2), 30);
		EXPECT_THROW((void)values.at(5), std::out_of_range);

		int* ptr = values.data();
		ASSERT_NE(ptr, nullptr);
		EXPECT_EQ(ptr[1], 20);
		ptr[1] = 99;
		EXPECT_EQ(values[1], 99);
	}

	TEST(ArrayHeader, IteratorFamilyCoversWholeRangeInOrder)
	{
		// begin/end and rbegin/rend provide bidirectional traversal over elements.
		std::array<int, 4> values{ 1, 2, 3, 4 };

		int forwardSum = 0;
		for (auto it = values.begin(); it != values.end(); ++it)
		{
			forwardSum += *it;
		}
		EXPECT_EQ(forwardSum, 10);

		int reverseConcat = 0;
		for (auto it = values.rbegin(); it != values.rend(); ++it)
		{
			reverseConcat = reverseConcat * 10 + *it;
		}
		EXPECT_EQ(reverseConcat, 4321);

		const std::array<int, 4>& cvalues = values;
		EXPECT_EQ(*cvalues.cbegin(), 1);
		EXPECT_EQ(*cvalues.crbegin(), 4);
	}

	TEST(ArrayHeader, EmptyAndMaxSizeForFixedExtentArrays)
	{
		// empty/max_size reflect fixed extent semantics of std::array.
		std::array<int, 3> nonEmpty{ 1, 2, 3 };
		EXPECT_FALSE(nonEmpty.empty());
		EXPECT_EQ(nonEmpty.max_size(), 3u);

		std::array<int, 0> empty{};
		EXPECT_TRUE(empty.empty());
		EXPECT_EQ(empty.size(), 0u);
		EXPECT_EQ(empty.max_size(), 0u);
	}

	TEST(ArrayHeader, FillAndSwapModifiers)
	{
		// fill assigns all elements to one value; swap exchanges full arrays.
		std::array<int, 4> a{ 1, 2, 3, 4 };
		std::array<int, 4> b{ 9, 8, 7, 6 };

		a.fill(5);
		EXPECT_EQ(a, (std::array<int, 4>{ 5, 5, 5, 5 }));

		a.swap(b);
		EXPECT_EQ(a, (std::array<int, 4>{ 9, 8, 7, 6 }));
		EXPECT_EQ(b, (std::array<int, 4>{ 5, 5, 5, 5 }));

		using std::swap;
		swap(a, b);
		EXPECT_EQ(a, (std::array<int, 4>{ 5, 5, 5, 5 }));
	}

	TEST(ArrayHeader, ComparisonOperatorsAreLexicographic)
	{
		// array comparisons are lexicographic and element-wise.
		std::array<int, 3> a{ 1, 2, 3 };
		std::array<int, 3> b{ 1, 2, 3 };
		std::array<int, 3> c{ 1, 2, 4 };

		EXPECT_TRUE(a == b);
		EXPECT_FALSE(a != b);
		EXPECT_TRUE(a != c);
		EXPECT_TRUE(a < c);
		EXPECT_TRUE(c > a);
		EXPECT_TRUE(a <= b);
		EXPECT_TRUE(c >= b);
	}

	TEST(ArrayHeader, GetAndTupleProtocolInteroperability)
	{
		// std::array is tuple-like: get, tuple_size, and tuple_element are provided.
		using A = std::array<long, 3>;
		EXPECT_EQ((std::tuple_size_v<A>), 3u);
		EXPECT_TRUE((std::is_same_v<std::tuple_element_t<0, A>, long>));
		EXPECT_TRUE((std::is_same_v<std::tuple_element_t<1, A>, long>));

		A values{ 11, 22, 33 };
		EXPECT_EQ(std::get<0>(values), 11);
		EXPECT_EQ(std::get<1>(values), 22);
		EXPECT_EQ(std::get<2>(values), 33);
	}

	TEST(ArrayHeader, ToArrayBuildsStdArrayFromBuiltInArrays)
	{
		// to_array converts built-in arrays to std::array with proper cvref behavior.
		int raw[] = { 3, 6, 9 };
		auto copied = std::to_array(raw);
		EXPECT_TRUE((std::is_same_v<decltype(copied), std::array<int, 3>>));
		EXPECT_EQ(copied[0], 3);
		EXPECT_EQ(copied[2], 9);

		const char text[] = "ok";
		auto textArr = std::to_array(text);
		EXPECT_EQ(textArr.size(), 3u); // includes null terminator
		EXPECT_EQ(textArr[0], 'o');
		EXPECT_EQ(textArr[1], 'k');
		EXPECT_EQ(textArr[2], '\0');
	}

}  // namespace
