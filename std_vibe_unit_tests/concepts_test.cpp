#include <gtest/gtest.h>

#include <concepts>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

namespace {

	struct Base {};
	struct Derived : Base {};

	struct RegularType
	{
		int value{};
		friend bool operator==(const RegularType&, const RegularType&) = default;
	};

	struct MoveOnly
	{
		MoveOnly() = default;
		MoveOnly(MoveOnly&&) = default;
		MoveOnly& operator=(MoveOnly&&) = default;
		MoveOnly(const MoveOnly&) = delete;
		MoveOnly& operator=(const MoveOnly&) = delete;
	};

	struct NonSwappable
	{
		NonSwappable() = default;
		NonSwappable(const NonSwappable&) = delete;
		NonSwappable& operator=(const NonSwappable&) = delete;
	};

	TEST(Concepts, TypeRelationshipConcepts)
	{
		// These concepts describe type identity/conversion and shared-reference
		// relationships used by generic APIs to constrain interoperable types.
		EXPECT_TRUE((std::same_as<int, int>));
		EXPECT_FALSE((std::same_as<int, long>));

		EXPECT_TRUE((std::derived_from<Derived, Base>));
		EXPECT_FALSE((std::derived_from<Base, Derived>));

		EXPECT_TRUE((std::convertible_to<int, double>));
		EXPECT_FALSE((std::convertible_to<std::string, int>));

		EXPECT_TRUE((std::common_reference_with<int&, const int&>));
		EXPECT_TRUE((std::common_with<int, long>));
	}

	TEST(Concepts, ArithmeticAndSignConcepts)
	{
		// <concepts> includes arithmetic category concepts that separate integral,
		// signed/unsigned integral, and floating-point types for overload control.
		EXPECT_TRUE((std::integral<int>));
		EXPECT_FALSE((std::integral<float>));

		EXPECT_TRUE((std::signed_integral<int>));
		EXPECT_FALSE((std::signed_integral<unsigned int>));

		EXPECT_TRUE((std::unsigned_integral<unsigned int>));
		EXPECT_FALSE((std::unsigned_integral<int>));

		EXPECT_TRUE((std::floating_point<double>));
		EXPECT_FALSE((std::floating_point<long long>));
	}

	TEST(Concepts, AssignmentConstructionAndLifetimeConcepts)
	{
		// These concepts model object semantics: assignability, destruction, and
		// construction/default initialization requirements.
		EXPECT_TRUE((std::assignable_from<int&, int>));
		EXPECT_FALSE((std::assignable_from<const int&, int>));

		EXPECT_TRUE((std::destructible<int>));
		EXPECT_TRUE((std::constructible_from<std::string, const char*>));
		EXPECT_TRUE((std::default_initializable<RegularType>));
		EXPECT_FALSE((std::default_initializable<int&>));
	}

	TEST(Concepts, MoveCopyAndRegularityConcepts)
	{
		// Move/copy hierarchy concepts are foundational for container and algorithm
		// requirements, culminating in semiregular/regular abstractions.
		EXPECT_TRUE((std::move_constructible<MoveOnly>));
		EXPECT_FALSE((std::copy_constructible<MoveOnly>));

		EXPECT_TRUE((std::movable<MoveOnly>));
		EXPECT_FALSE((std::copyable<MoveOnly>));

		EXPECT_TRUE((std::semiregular<RegularType>));
		EXPECT_TRUE((std::regular<RegularType>));
	}

	TEST(Concepts, SwappabilityConcepts)
	{
		// swappable/swappable_with constrain algorithms that reorder values and rely
		// on ADL-enabled swap contracts.
		EXPECT_TRUE((std::swappable<int>));
		EXPECT_TRUE((std::swappable_with<int&, int&>));

		EXPECT_FALSE((std::swappable<NonSwappable>));
		EXPECT_FALSE((std::swappable_with<NonSwappable&, NonSwappable&>));
	}

	TEST(Concepts, EqualityAndTotalOrderingConcepts)
	{
		// Equality and total-order concepts formalize comparison guarantees needed by
		// associative containers, sorting, and ordering-based generic interfaces.
		EXPECT_TRUE((std::equality_comparable<int>));
		EXPECT_TRUE((std::equality_comparable_with<int, long>));

		EXPECT_TRUE((std::totally_ordered<int>));
		EXPECT_TRUE((std::totally_ordered_with<int, long>));
	}

	TEST(Concepts, CallableAndPredicateFamilyConcepts)
	{
		// Callable concepts capture increasing semantic strength from "invocable" to
		// predicates and binary relations, enabling precise function constraints.
		auto unary = [](int v) { return v + 1; };
		auto isEven = [](int v) { return (v % 2) == 0; };
		auto less = [](int lhs, int rhs) { return lhs < rhs; };
		auto equivalentByParity = [](int lhs, int rhs) { return (lhs % 2) == (rhs % 2); };

		EXPECT_TRUE((std::invocable<decltype(unary), int>));
		EXPECT_TRUE((std::regular_invocable<decltype(unary), int>));
		EXPECT_TRUE((std::predicate<decltype(isEven), int>));

		EXPECT_TRUE((std::relation<decltype(less), int, int>));
		EXPECT_TRUE((std::strict_weak_order<decltype(less), int, int>));
		EXPECT_TRUE((std::equivalence_relation<decltype(equivalentByParity), int, int>));
	}

}  // namespace
