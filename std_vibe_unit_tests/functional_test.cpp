#include <gtest/gtest.h>

#include <functional>
#include <string>
#include <type_traits>
#include <utility>

namespace {

	struct Calculator
	{
		int factor = 0;
		int Multiply(int v) const { return factor * v; }
		int Add(int a, int b) const { return a + b + factor; }
	};

	int FreeAdd(int a, int b)
	{
		return a + b;
	}

	TEST(Functional, ReferenceWrapperRefAndCrefBehavior)
	{
		// reference_wrapper stores copyable references and is created by ref/cref.
		int value = 10;
		std::reference_wrapper<int> rw = std::ref(value);
		std::reference_wrapper<const int> crw = std::cref(value);

		rw.get() = 42;
		EXPECT_EQ(value, 42);
		EXPECT_EQ(crw.get(), 42);
		EXPECT_TRUE((std::is_same_v<decltype(rw.get()), int&>));
	}

	TEST(Functional, InvokeSupportsFreeFunctionsMemberFunctionsAndData)
	{
		// std::invoke unifies callable invocation across free functions, member
		// functions, member data pointers, lambdas, and function objects.
		Calculator calc{ 3 };
		EXPECT_EQ(std::invoke(FreeAdd, 2, 5), 7);
		EXPECT_EQ(std::invoke(&Calculator::Multiply, calc, 4), 12);
		EXPECT_EQ(std::invoke(&Calculator::factor, calc), 3);

		auto lambda = [](int x) { return x * x; };
		EXPECT_EQ(std::invoke(lambda, 6), 36);
	}

	TEST(Functional, InvokeRConvertsResultToRequestedType)
	{
		// invoke_r (C++23) invokes a callable then converts to requested return type.
#ifdef __cpp_lib_invoke_r
		EXPECT_GE(__cpp_lib_invoke_r, 202106L);
		const double result = std::invoke_r<double>(FreeAdd, 2, 3);
		EXPECT_DOUBLE_EQ(result, 5.0);
#else
		GTEST_SKIP() << "std::invoke_r is not available on this toolchain.";
#endif
	}

	TEST(Functional, FunctionTypeErasureAndTargetIntrospection)
	{
		// std::function type-erases callable objects with a fixed signature.
		std::function<int(int, int)> fn = FreeAdd;
		EXPECT_TRUE(static_cast<bool>(fn));
		EXPECT_EQ(fn(4, 6), 10);

		fn = [](int a, int b) { return a * b; };
		EXPECT_EQ(fn(4, 6), 24);

		using FnType = int(*)(int, int);
		EXPECT_EQ(fn.target<FnType>(), nullptr); // currently stores lambda

		fn = FreeAdd;
		ASSERT_NE(fn.target<FnType>(), nullptr);
		EXPECT_EQ((*fn.target<FnType>())(1, 2), 3);
	}

	TEST(Functional, BindAndPlaceholdersCreateDeferredCallWrappers)
	{
		// std::bind creates a callable with bound arguments and placeholders.
		using namespace std::placeholders;
		auto bound = std::bind(FreeAdd, _2, _1);
		EXPECT_EQ(bound(10, 3), 13);

		Calculator calc{ 5 };
		auto memberBound = std::bind(&Calculator::Add, calc, _1, 7);
		EXPECT_EQ(memberBound(4), 16);

		EXPECT_TRUE((std::is_bind_expression_v<decltype(bound)>));
		EXPECT_EQ(std::is_placeholder_v<decltype(_1)>, 1);
		EXPECT_EQ(std::is_placeholder_v<decltype(_2)>, 2);
	}

	TEST(Functional, MemFnAdaptsMemberPointersToCallables)
	{
		// std::mem_fn turns member pointers into generic callables.
		Calculator calc{ 8 };
		auto mult = std::mem_fn(&Calculator::Multiply);
		auto data = std::mem_fn(&Calculator::factor);

		EXPECT_EQ(mult(calc, 2), 16);
		EXPECT_EQ(data(calc), 8);
	}

	TEST(Functional, NotFnNegatesPredicateResults)
	{
		// std::not_fn returns a callable that negates the wrapped callable result.
		auto isEven = [](int v) { return (v % 2) == 0; };
		auto isOdd = std::not_fn(isEven);
		EXPECT_TRUE(isOdd(3));
		EXPECT_FALSE(isOdd(4));
	}

	TEST(Functional, ArithmeticComparisonLogicalAndBitwiseFunctionObjects)
	{
		// <functional> defines stateless function objects for arithmetic, relation,
		// logic, and bitwise operations.
		EXPECT_EQ(std::plus<int>{}(2, 3), 5);
		EXPECT_EQ(std::minus<int>{}(7, 3), 4);
		EXPECT_EQ(std::multiplies<int>{}(4, 5), 20);
		EXPECT_EQ(std::divides<int>{}(20, 4), 5);
		EXPECT_EQ(std::modulus<int>{}(22, 5), 2);
		EXPECT_EQ(std::negate<int>{}(6), -6);

		EXPECT_TRUE(std::equal_to<int>{}(10, 10));
		EXPECT_TRUE(std::not_equal_to<int>{}(10, 11));
		EXPECT_TRUE(std::greater<int>{}(7, 3));
		EXPECT_TRUE(std::less<int>{}(3, 7));
		EXPECT_TRUE(std::greater_equal<int>{}(5, 5));
		EXPECT_TRUE(std::less_equal<int>{}(5, 5));

		EXPECT_TRUE(std::logical_and<bool>{}(true, true));
		EXPECT_TRUE(std::logical_or<bool>{}(false, true));
		EXPECT_TRUE(std::logical_not<bool>{}(false));

		EXPECT_EQ(std::bit_and<unsigned>{}(0b1100u, 0b1010u), 0b1000u);
		EXPECT_EQ(std::bit_or<unsigned>{}(0b1100u, 0b1010u), 0b1110u);
		EXPECT_EQ(std::bit_xor<unsigned>{}(0b1100u, 0b1010u), 0b0110u);
		EXPECT_EQ(std::bit_not<unsigned>{}(0u), ~0u);
	}

	TEST(Functional, IdentityAndHashUtilities)
	{
		// identity returns input unchanged and hash computes hash codes.
		std::string s = "vibe";
		auto&& ref = std::identity{}(s);
		EXPECT_EQ(ref, "vibe");
		EXPECT_TRUE((std::is_lvalue_reference_v<decltype(ref)>));

		const std::size_t h1 = std::hash<std::string>{}(s);
		const std::size_t h2 = std::hash<std::string>{}(s);
		EXPECT_EQ(h1, h2);
	}

	TEST(Functional, BindFrontAndBindBackWhenAvailable)
	{
		// bind_front/bind_back provide lighter-weight partial application helpers.
#ifdef __cpp_lib_bind_front
		EXPECT_GE(__cpp_lib_bind_front, 201907L);
		auto addFront = std::bind_front(FreeAdd, 10);
		EXPECT_EQ(addFront(4), 14);
#else
		GTEST_SKIP() << "std::bind_front is not available.";
#endif
	}

	TEST(Functional, BindBackWhenAvailable)
	{
#ifdef __cpp_lib_bind_back
		EXPECT_GE(__cpp_lib_bind_back, 202202L);
		auto addBack = std::bind_back(FreeAdd, 10);
		EXPECT_EQ(addBack(4), 14);
#else
		GTEST_SKIP() << "std::bind_back is not available.";
#endif
	}

}  // namespace
