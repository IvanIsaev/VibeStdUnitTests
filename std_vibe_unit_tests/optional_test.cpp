#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace {

	struct MoveOnly
	{
		int value = 0;
		explicit MoveOnly(int v) : value(v) {}
		MoveOnly(MoveOnly&&) noexcept = default;
		MoveOnly& operator=(MoveOnly&&) noexcept = default;
		MoveOnly(const MoveOnly&) = delete;
		MoveOnly& operator=(const MoveOnly&) = delete;
	};

	TEST(OptionalHeader, NulloptTagAndDefaultConstructionProduceDisengagedState)
	{
		// std::nullopt_t/std::nullopt represent the "no value" tag. Default
		// construction and nullopt construction both produce disengaged optionals.
		EXPECT_TRUE((std::is_same_v<decltype(std::nullopt), const std::nullopt_t>));

		std::optional<int> a;
		std::optional<int> b(std::nullopt);
		EXPECT_FALSE(a.has_value());
		EXPECT_FALSE(b.has_value());
		EXPECT_EQ(static_cast<bool>(a), false);
	}

	TEST(OptionalHeader, ValueConstructionAssignmentAndObservers)
	{
		// optional can be engaged from a value, reassigned, and observed via
		// has_value/operator bool/value/operator*/operator->.
		std::optional<std::string> value("hello");
		EXPECT_TRUE(value.has_value());
		EXPECT_TRUE(static_cast<bool>(value));
		EXPECT_EQ(value.value(), "hello");
		EXPECT_EQ(*value, "hello");
		EXPECT_EQ(value->size(), 5u);

		value = std::string("world");
		EXPECT_EQ(value.value(), "world");
	}

	TEST(OptionalHeader, ValueAccessThrowsBadOptionalAccessWhenDisengaged)
	{
		// Calling value() on disengaged optional throws bad_optional_access.
		std::optional<int> empty;
		EXPECT_THROW((void)empty.value(), std::bad_optional_access);
		try
		{
			(void)empty.value();
			FAIL() << "Expected std::bad_optional_access.";
		}
		catch (const std::bad_optional_access& ex)
		{
			EXPECT_NE(*ex.what(), '\0');
		}
		catch (...)
		{
			FAIL() << "Unexpected exception type.";
		}
	}

	TEST(OptionalHeader, ValueOrReturnsContainedValueOrFallback)
	{
		// value_or returns contained value when engaged, otherwise fallback.
		std::optional<int> engaged(7);
		std::optional<int> disengaged;
		EXPECT_EQ(engaged.value_or(100), 7);
		EXPECT_EQ(disengaged.value_or(100), 100);
	}

	TEST(OptionalHeader, EmplaceResetAndSwapManageEngagementState)
	{
		// emplace constructs in place, reset disengages, and swap exchanges states.
		std::optional<std::string> a;
		std::optional<std::string> b("beta");

		auto& ref = a.emplace(3, 'x');
		EXPECT_EQ(ref, "xxx");
		EXPECT_TRUE(a.has_value());

		a.swap(b);
		EXPECT_EQ(a.value(), "beta");
		EXPECT_EQ(b.value(), "xxx");

		b.reset();
		EXPECT_FALSE(b.has_value());
	}

	TEST(OptionalHeader, MakeOptionalFactoriesConstructEngagedOptionals)
	{
		// make_optional creates engaged optionals and supports type-deduced and
		// explicit-type construction forms.
		auto a = std::make_optional(42);
		EXPECT_TRUE((std::is_same_v<decltype(a), std::optional<int>>));
		EXPECT_EQ(a.value(), 42);

		auto s = std::make_optional<std::string>(4, 'z');
		EXPECT_EQ(s.value(), "zzzz");
	}

	TEST(OptionalHeader, ComparisonOperatorsWithOptionalNulloptAndValue)
	{
		// optional supports comparisons with optional, nullopt, and value.
		std::optional<int> a(5);
		std::optional<int> b(5);
		std::optional<int> c(9);
		std::optional<int> empty;

		EXPECT_TRUE(a == b);
		EXPECT_TRUE(a != c);
		EXPECT_TRUE(a < c);
		EXPECT_TRUE(c > a);
		EXPECT_TRUE(empty == std::nullopt);
		EXPECT_TRUE(a != std::nullopt);
		EXPECT_TRUE(a == 5);
		EXPECT_TRUE(a < 6);
	}

	TEST(OptionalHeader, OptionalSupportsMoveOnlyTypesThroughEmplaceAndMove)
	{
		// optional works with move-only payload types as long as operations respect
		// move semantics.
		std::optional<MoveOnly> m(std::in_place, 77);
		EXPECT_TRUE(m.has_value());
		EXPECT_EQ(m->value, 77);

		std::optional<MoveOnly> moved(std::move(m));
		EXPECT_TRUE(moved.has_value());
		EXPECT_EQ(moved->value, 77);
	}

	TEST(OptionalHeader, HashSupportForOptionalWhenPayloadIsHashable)
	{
		// std::hash specialization for optional<T> is provided when T is hashable.
		const std::optional<int> a(10);
		const std::optional<int> b(10);
		const std::optional<int> empty;
		EXPECT_EQ(std::hash<std::optional<int>>{}(a), std::hash<std::optional<int>>{}(b));
		EXPECT_NE(std::hash<std::optional<int>>{}(a), std::hash<std::optional<int>>{}(empty));
	}

	TEST(OptionalHeader, MonadicOperationsWhenAvailable)
	{
		// C++23 monadic operations provide pipeline composition for optional.
#ifdef __cpp_lib_optional
		EXPECT_GE(__cpp_lib_optional, 202110L);
		std::optional<int> ok(5);
		std::optional<int> empty;

		auto transformed = ok.transform([](int v) { return v * 2; });
		EXPECT_TRUE(transformed.has_value());
		EXPECT_EQ(transformed.value(), 10);

		auto andThen = ok.and_then([](int v) -> std::optional<std::string>
		{
			return std::string(v, 'a');
		});
		EXPECT_TRUE(andThen.has_value());
		EXPECT_EQ(andThen.value(), "aaaaa");

		auto fallback = empty.or_else([]() -> std::optional<int>
		{
			return 42;
		});
		EXPECT_TRUE(fallback.has_value());
		EXPECT_EQ(fallback.value(), 42);
#else
		GTEST_SKIP() << "Monadic optional operations are not available.";
#endif
	}

}  // namespace
