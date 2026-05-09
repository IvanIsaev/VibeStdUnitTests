#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <variant>

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

	TEST(VariantHeader, VariantSizeAndAlternativeTypeIntrospection)
	{
		// variant_size/variant_alternative expose compile-time arity and element
		// type lookup for std::variant.
		using V = std::variant<int, double, std::string>;
		EXPECT_EQ((std::variant_size_v<V>), 3u);
		EXPECT_TRUE((std::is_same_v<std::variant_alternative_t<0, V>, int>));
		EXPECT_TRUE((std::is_same_v<std::variant_alternative_t<1, V>, double>));
		EXPECT_TRUE((std::is_same_v<std::variant_alternative_t<2, V>, std::string>));
	}

	TEST(VariantHeader, DefaultConstructionAndMonostateBehavior)
	{
		// Default construction initializes first alternative. monostate is commonly
		// used as explicit empty/default-first alternative.
		std::variant<std::monostate, int> v;
		EXPECT_TRUE(std::holds_alternative<std::monostate>(v));
		EXPECT_EQ(v.index(), 0u);

		std::monostate a{};
		std::monostate b{};
		EXPECT_TRUE(a == b);
		EXPECT_FALSE(a != b);
	}

	TEST(VariantHeader, ValueConstructionAssignmentAndIndexTracking)
	{
		// variant can switch active alternatives via construction and assignment.
		std::variant<int, std::string> v(42);
		EXPECT_TRUE(std::holds_alternative<int>(v));
		EXPECT_EQ(v.index(), 0u);
		EXPECT_EQ(std::get<int>(v), 42);

		v = std::string("hello");
		EXPECT_TRUE(std::holds_alternative<std::string>(v));
		EXPECT_EQ(v.index(), 1u);
		EXPECT_EQ(std::get<std::string>(v), "hello");
	}

	TEST(VariantHeader, InPlaceConstructionByIndexAndType)
	{
		// in_place_index/in_place_type tags select explicit alternative and forward
		// constructor arguments directly into the active object.
		std::variant<int, std::string> a(std::in_place_index<1>, 3, 'x');
		EXPECT_TRUE(std::holds_alternative<std::string>(a));
		EXPECT_EQ(std::get<1>(a), "xxx");

		std::variant<int, std::string> b(std::in_place_type<std::string>, "world");
		EXPECT_TRUE(std::holds_alternative<std::string>(b));
		EXPECT_EQ(std::get<std::string>(b), "world");
	}

	TEST(VariantHeader, GetAndGetIfAccessorsAndTypeMismatchBehavior)
	{
		// get<T>/get<I> access active alternative and throw bad_variant_access on
		// mismatch; get_if returns null pointer instead of throwing.
		std::variant<int, std::string> v(std::string("abc"));
		EXPECT_EQ(std::get<std::string>(v), "abc");
		EXPECT_EQ(std::get<1>(v), "abc");

		EXPECT_THROW((void)std::get<int>(v), std::bad_variant_access);
		EXPECT_THROW((void)std::get<0>(v), std::bad_variant_access);

		EXPECT_EQ(std::get_if<int>(&v), nullptr);
		const std::string* s = std::get_if<std::string>(&v);
		ASSERT_NE(s, nullptr);
		EXPECT_EQ(*s, "abc");
	}

	TEST(VariantHeader, VisitDispatchesBasedOnActiveAlternative)
	{
		// visit performs type-safe runtime dispatch over active alternatives.
		std::variant<int, std::string> v = 10;
		const std::string first = std::visit([](const auto& value) -> std::string
		{
			using T = std::decay_t<decltype(value)>;
			if constexpr (std::is_same_v<T, int>)
			{
				return "int:" + std::to_string(value);
			}
			else
			{
				return "str:" + value;
			}
		}, v);
		EXPECT_EQ(first, "int:10");

		v = std::string("xyz");
		const std::string second = std::visit([](const auto& value) -> std::string
		{
			using T = std::decay_t<decltype(value)>;
			if constexpr (std::is_same_v<T, int>)
			{
				return "int:" + std::to_string(value);
			}
			else
			{
				return "str:" + value;
			}
		}, v);
		EXPECT_EQ(second, "str:xyz");
	}

	TEST(VariantHeader, MultiVariantVisitCombinesAlternatives)
	{
		// visit accepts multiple variants and dispatches on Cartesian combination
		// of active alternatives.
		std::variant<int, std::string> a = 5;
		std::variant<int, std::string> b = std::string("q");

		const std::string combined = std::visit(
			[](const auto& lhs, const auto& rhs) -> std::string
			{
				return std::to_string(static_cast<int>(std::is_integral_v<std::decay_t<decltype(lhs)>>)) +
				       std::to_string(static_cast<int>(std::is_integral_v<std::decay_t<decltype(rhs)>>));
			},
			a, b);
		EXPECT_EQ(combined, "10");
	}

	TEST(VariantHeader, ComparisonOperatorsAndSwap)
	{
		// variant supports relational/equality comparisons and swap.
		std::variant<int, std::string> a = 7;
		std::variant<int, std::string> b = 9;
		std::variant<int, std::string> c = std::string("x");

		EXPECT_TRUE(a != b);
		EXPECT_TRUE(a < b);
		EXPECT_TRUE(c > a); // index 1 alternative is ordered after index 0

		a.swap(c);
		EXPECT_TRUE(std::holds_alternative<std::string>(a));
		EXPECT_TRUE(std::holds_alternative<int>(c));
		EXPECT_EQ(std::get<std::string>(a), "x");
		EXPECT_EQ(std::get<int>(c), 7);
	}

	TEST(VariantHeader, EmplaceAndMoveOnlyAlternativeSupport)
	{
		// emplace constructs selected alternative in place and supports move-only
		// types as alternatives.
		std::variant<int, MoveOnly> v(std::in_place_index<1>, 33);
		EXPECT_TRUE(std::holds_alternative<MoveOnly>(v));
		EXPECT_EQ(std::get<MoveOnly>(v).value, 33);

		v.emplace<0>(123);
		EXPECT_TRUE(std::holds_alternative<int>(v));
		EXPECT_EQ(std::get<int>(v), 123);
	}

	TEST(VariantHeader, ValuelessByExceptionIsUsuallyFalseInNormalUsage)
	{
		// valueless_by_exception indicates rare state after exception during
		// alternative-changing operation. In normal non-throwing paths it is false.
		std::variant<int, std::string> v = 1;
		EXPECT_FALSE(v.valueless_by_exception());
		v = std::string("safe");
		EXPECT_FALSE(v.valueless_by_exception());
	}

}  // namespace
