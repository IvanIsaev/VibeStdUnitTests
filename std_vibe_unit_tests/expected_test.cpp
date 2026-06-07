#include <gtest/gtest.h>

#include <expected>
#include <string>
#include <type_traits>
#include <utility>

namespace {

	TEST(ExpectedHeader, FeatureMacroAndTagTypesAreAvailable)
	{
		// <expected> exposes feature-test macro plus tag types used to construct
		// error states directly.
#ifdef __cpp_lib_expected
		EXPECT_GE(__cpp_lib_expected, 202202L);
#endif
		EXPECT_TRUE((std::is_same_v<decltype(std::unexpect), const std::unexpect_t>));
	}

	TEST(ExpectedHeader, UnexpectedStoresAndExposesErrorValue)
	{
		// std::unexpected<E> is a lightweight wrapper for error payloads.
		std::unexpected<std::string> u1("failed");
		EXPECT_EQ(u1.error(), "failed");

		std::unexpected<std::string> u2(std::in_place, 3, 'x');
		EXPECT_EQ(u2.error(), "xxx");

		EXPECT_TRUE(u1 != u2);
		u2 = std::unexpected<std::string>("failed");
		EXPECT_TRUE(u1 == u2);
	}

	TEST(ExpectedHeader, ExpectedValueStateObserversAndAccessors)
	{
		// expected<T, E> can hold either value or error. In value state, has_value
		// is true and value/operator* access the contained success object.
		std::expected<int, std::string> value(42);
		EXPECT_TRUE(value.has_value());
		EXPECT_TRUE(static_cast<bool>(value));
		EXPECT_EQ(value.value(), 42);
		EXPECT_EQ(*value, 42);
	}

	TEST(ExpectedHeader, ExpectedErrorStateObserversAndErrorAccessors)
	{
		// In error state, has_value is false and error() provides the error payload.
		std::expected<int, std::string> err(std::unexpect, "boom");
		EXPECT_FALSE(err.has_value());
		EXPECT_FALSE(static_cast<bool>(err));
		EXPECT_EQ(err.error(), "boom");
	}

	TEST(ExpectedHeader, ValueOrAndErrorOrFallbackBehavior)
	{
		// value_or returns stored value in success case, otherwise fallback value.
		std::expected<int, std::string> ok(7);
		std::expected<int, std::string> bad(std::unexpect, "nope");
		EXPECT_EQ(ok.value_or(100), 7);
		EXPECT_EQ(bad.value_or(100), 100);
	}

	TEST(ExpectedHeader, ValueThrowsBadExpectedAccessWhenInErrorState)
	{
		// Calling value() on an error-state expected throws bad_expected_access<E>.
		std::expected<int, std::string> bad(std::unexpect, "explode");
		try
		{
			(void)bad.value();
			FAIL() << "Expected bad_expected_access<std::string>.";
		}
		catch (const std::bad_expected_access<std::string>& ex)
		{
			EXPECT_EQ(ex.error(), "explode");
		}
		catch (...)
		{
			FAIL() << "Unexpected exception type.";
		}
	}

	TEST(ExpectedHeader, EmplaceSwapAndAssignmentTransitionStates)
	{
		// expected supports assignment between states, emplacement of value, and swap
		// while preserving algebra of value/error alternatives.
		std::expected<std::string, int> a("alpha");
		std::expected<std::string, int> b(std::unexpect, 9);

		a.swap(b);
		EXPECT_FALSE(a.has_value());
		EXPECT_TRUE(b.has_value());
		EXPECT_EQ(a.error(), 9);
		EXPECT_EQ(b.value(), "alpha");

		a = std::string("beta");
		EXPECT_TRUE(a.has_value());
		EXPECT_EQ(a.value(), "beta");

		b = std::unexpected<int>(5);
		EXPECT_FALSE(b.has_value());
		EXPECT_EQ(b.error(), 5);

		// TODO: Fix
		//auto& ref = b.emplace("gamma");
		//EXPECT_EQ(ref, "gamma");
		//EXPECT_TRUE(b.has_value());
	}

	TEST(ExpectedHeader, EqualityComparisonWithExpectedAndUnexpected)
	{
		// expected supports comparisons against other expected instances and
		// unexpected wrappers when error types align.
		std::expected<int, std::string> ok1(1);
		std::expected<int, std::string> ok2(1);
		std::expected<int, std::string> bad(std::unexpect, "err");
		std::unexpected<std::string> u("err");

		EXPECT_TRUE(ok1 == ok2);
		EXPECT_TRUE(bad == u);
		EXPECT_TRUE(u == bad);
		EXPECT_TRUE(ok1 != bad);
	}

	TEST(ExpectedHeader, ExpectedVoidSpecializationSupportsSuccessAndError)
	{
		// expected<void, E> models operations that return success/failure without a
		// payload for successful path, while still carrying rich error information.
		std::expected<void, int> ok;
		EXPECT_TRUE(ok.has_value());
		EXPECT_NO_THROW(ok.value());

		std::expected<void, int> bad(std::unexpect, 77);
		EXPECT_FALSE(bad.has_value());
		EXPECT_EQ(bad.error(), 77);
		EXPECT_THROW((void)bad.value(), std::bad_expected_access<int>);
	}

	TEST(ExpectedHeader, MonadicOperationsTransformAndThenOrElseAndTransformError)
	{
		// C++23 monadic helpers provide pipeline-style composition of expected-based
		// operations for success and error branches.
		std::expected<int, std::string> ok(5);
		std::expected<int, std::string> bad(std::unexpect, "x");

		auto doubled = ok.transform([](int v) { return v * 2; });
		EXPECT_TRUE(doubled.has_value());
		EXPECT_EQ(doubled.value(), 10);

		auto chained = ok.and_then([](int v) -> std::expected<std::string, std::string>
		{
			return std::string(v, 'a');
		});
		EXPECT_TRUE(chained.has_value());
		EXPECT_EQ(chained.value(), "aaaaa");

		auto recovered = bad.or_else([](const std::string& e) -> std::expected<int, std::string>
		{
			return static_cast<int>(e.size());
		});
		EXPECT_TRUE(recovered.has_value());
		EXPECT_EQ(recovered.value(), 1);

		auto remappedError = bad.transform_error([](const std::string& e)
		{
			return static_cast<int>(e.size());
		});
		EXPECT_FALSE(remappedError.has_value());
		EXPECT_EQ(remappedError.error(), 1);
	}

}  // namespace
