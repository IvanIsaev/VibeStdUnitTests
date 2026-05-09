#include <gtest/gtest.h>

#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace {

	struct Aggregate
	{
		int id;
		std::string name;
		double score;
	};

	TEST(Tuple, TupleConstructionAndElementAccessByIndexAndType)
	{
		// std::tuple stores heterogenous values. std::get works by index and by type
		// (type form requires uniqueness inside the tuple).
		std::tuple<int, std::string, double> t(7, "alpha", 2.5);
		EXPECT_EQ(std::get<0>(t), 7);
		EXPECT_EQ(std::get<1>(t), "alpha");
		EXPECT_DOUBLE_EQ(std::get<2>(t), 2.5);

		EXPECT_EQ(std::get<int>(t), 7);
		EXPECT_EQ(std::get<std::string>(t), "alpha");
		EXPECT_DOUBLE_EQ(std::get<double>(t), 2.5);
	}

	TEST(Tuple, TupleSizeAndTupleElementTypeIntrospection)
	{
		// tuple_size/tuple_element expose compile-time tuple arity and element types.
		using T = std::tuple<int, long, const char*>;
		EXPECT_EQ((std::tuple_size_v<T>), 3u);
		EXPECT_TRUE((std::is_same_v<std::tuple_element_t<0, T>, int>));
		EXPECT_TRUE((std::is_same_v<std::tuple_element_t<1, T>, long>));
		EXPECT_TRUE((std::is_same_v<std::tuple_element_t<2, T>, const char*>));
	}

	TEST(Tuple, MakeTupleCapturesValuesAndDecaysReferences)
	{
		// make_tuple performs type decay and creates value-owned tuple elements.
		int x = 5;
		const int& cref = x;
		auto t = std::make_tuple(cref, "hello");
		EXPECT_TRUE((std::is_same_v<decltype(t), std::tuple<int, const char*>>));
		EXPECT_EQ(std::get<0>(t), 5);
		EXPECT_STREQ(std::get<1>(t), "hello");
	}

	TEST(Tuple, TieCreatesReferenceTupleForStructuredAssignment)
	{
		// tie creates a tuple of lvalue references, useful for unpacking and
		// assignment from tuple/pair-producing operations.
		int a = 0;
		double b = 0.0;
		std::string c;

		std::tie(a, b, c) = std::make_tuple(10, 3.14, std::string("vibe"));
		EXPECT_EQ(a, 10);
		EXPECT_DOUBLE_EQ(b, 3.14);
		EXPECT_EQ(c, "vibe");
	}

	TEST(Tuple, IgnoreCanSkipSelectedValuesInTieAssignment)
	{
		// std::ignore is an assignable sink used with tie when only subset of values
		// should be captured.
		int value = 0;
		std::string kept;
		std::tie(value, std::ignore, kept) = std::make_tuple(1, 999, std::string("kept"));
		EXPECT_EQ(value, 1);
		EXPECT_EQ(kept, "kept");
	}

	TEST(Tuple, ForwardAsTuplePreservesValueCategories)
	{
		// forward_as_tuple creates a tuple of forwarding references without copying.
		int x = 10;
		auto t = std::forward_as_tuple(x, 42);
		EXPECT_TRUE((std::is_lvalue_reference_v<decltype(std::get<0>(t))>));
		EXPECT_TRUE((std::is_rvalue_reference_v<decltype(std::get<1>(t))>));
	}

	TEST(Tuple, TupleCatConcatenatesTuplesAndPairLikeTypes)
	{
		// tuple_cat concatenates tuples (and tuple-like objects such as pair) into a
		// single tuple preserving element order.
		auto t1 = std::make_tuple(1, std::string("a"));
		std::pair<double, char> p{ 2.5, 'z' };
		auto t2 = std::make_tuple(true);

		auto cat = std::tuple_cat(t1, p, t2);
		EXPECT_TRUE((std::is_same_v<decltype(cat), std::tuple<int, std::string, double, char, bool>>));
		EXPECT_EQ(std::get<0>(cat), 1);
		EXPECT_EQ(std::get<1>(cat), "a");
		EXPECT_DOUBLE_EQ(std::get<2>(cat), 2.5);
		EXPECT_EQ(std::get<3>(cat), 'z');
		EXPECT_EQ(std::get<4>(cat), true);
	}

	TEST(Tuple, ApplyInvokesCallableWithTupleElements)
	{
		// apply expands tuple elements into callable arguments.
		auto values = std::make_tuple(3, 4, 5);
		const int sum = std::apply([](int a, int b, int c)
		{
			return a + b + c;
		}, values);
		EXPECT_EQ(sum, 12);
	}

	TEST(Tuple, MakeFromTupleConstructsObjectFromTupleArguments)
	{
		// make_from_tuple forwards tuple elements to a constructor call.
		auto args = std::make_tuple(17, std::string("name"), 9.75);
		Aggregate obj = std::make_from_tuple<Aggregate>(args);
		EXPECT_EQ(obj.id, 17);
		EXPECT_EQ(obj.name, "name");
		EXPECT_DOUBLE_EQ(obj.score, 9.75);
	}

	TEST(Tuple, SwapAndComparisonOperators)
	{
		// tuple supports lexicographic comparisons and swap.
		std::tuple<int, int> a{ 1, 2 };
		std::tuple<int, int> b{ 3, 4 };
		EXPECT_TRUE(a < b);
		EXPECT_TRUE(b > a);
		EXPECT_TRUE(a != b);

		a.swap(b);
		EXPECT_EQ(a, (std::tuple<int, int>{ 3, 4 }));
		EXPECT_EQ(b, (std::tuple<int, int>{ 1, 2 }));

		using std::swap;
		swap(a, b);
		EXPECT_EQ(a, (std::tuple<int, int>{ 1, 2 }));
		EXPECT_EQ(b, (std::tuple<int, int>{ 3, 4 }));
	}

	TEST(Tuple, PairInteroperabilityViaTupleProtocol)
	{
		// std::pair participates in tuple protocol and can be manipulated through
		// tuple_size/tuple_element/get.
		using P = std::pair<int, std::string>;
		EXPECT_EQ((std::tuple_size_v<P>), 2u);
		EXPECT_TRUE((std::is_same_v<std::tuple_element_t<0, P>, int>));
		EXPECT_TRUE((std::is_same_v<std::tuple_element_t<1, P>, std::string>));

		P p{ 9, "nine" };
		EXPECT_EQ(std::get<0>(p), 9);
		EXPECT_EQ(std::get<1>(p), "nine");
	}

}  // namespace
