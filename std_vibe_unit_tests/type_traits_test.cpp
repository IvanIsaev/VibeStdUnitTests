#include <gtest/gtest.h>

#include <exception>
#include <type_traits>
#include <utility>

namespace {

	struct Base {};
	struct Derived : Base {};
	enum class ScopedEnum : int { A = 1, B = 2 };
	enum UnscopedEnum { X = 1, Y = 2 };
	union DemoUnion
	{
		int i;
		double d;
	};

	struct TrivialType
	{
		int v;
	};

	struct NonTrivialDtor
	{
		~NonTrivialDtor() {}
	};

	struct NothrowMove
	{
		NothrowMove() = default;
		NothrowMove(NothrowMove&&) noexcept = default;
		NothrowMove& operator=(NothrowMove&&) noexcept = default;
	};

	struct ThrowsMove
	{
		ThrowsMove() = default;
		ThrowsMove(ThrowsMove&&) {}
		ThrowsMove& operator=(ThrowsMove&&) { return *this; }
	};

	struct EmptyType {};
struct FinalType final {};
	struct StandardLayoutType
	{
		int a;
		double b;
	};

	TEST(TypeTraits, FundamentalTypeCategoryTraits)
	{
		// Primary category traits identify broad type classes used by generic code.
		EXPECT_TRUE((std::is_void_v<void>));
		EXPECT_TRUE((std::is_null_pointer_v<std::nullptr_t>));
		EXPECT_TRUE((std::is_integral_v<int>));
		EXPECT_TRUE((std::is_floating_point_v<double>));
		EXPECT_TRUE((std::is_array_v<int[3]>));
		EXPECT_TRUE((std::is_enum_v<ScopedEnum>));
		EXPECT_TRUE((std::is_union_v<DemoUnion>));
		EXPECT_TRUE((std::is_class_v<Derived>));
		EXPECT_TRUE((std::is_function_v<int(int)>));
		EXPECT_TRUE((std::is_pointer_v<int*>));
		EXPECT_TRUE((std::is_lvalue_reference_v<int&>));
		EXPECT_TRUE((std::is_rvalue_reference_v<int&&>));
		EXPECT_TRUE((std::is_member_object_pointer_v<int Base::*>));
		EXPECT_TRUE((std::is_member_function_pointer_v<void (Base::*)()>));
		EXPECT_TRUE((std::is_arithmetic_v<long long>));
		EXPECT_TRUE((std::is_fundamental_v<char>));
		EXPECT_TRUE((std::is_object_v<Derived>));
		EXPECT_TRUE((std::is_scalar_v<int*>));
		EXPECT_TRUE((std::is_compound_v<int&>));
		EXPECT_TRUE((std::is_reference_v<const int&>));
		EXPECT_TRUE((std::is_member_pointer_v<int Base::*>));
	}

	TEST(TypeTraits, ConstVolatileAndSignednessTraits)
	{
		// CV and sign traits support precise branching and SFINAE constraints.
		EXPECT_TRUE((std::is_const_v<const int>));
		EXPECT_TRUE((std::is_volatile_v<volatile int>));
		EXPECT_TRUE((std::is_signed_v<int>));
		EXPECT_TRUE((std::is_unsigned_v<unsigned int>));
		EXPECT_TRUE((std::is_bounded_array_v<int[4]>));
		EXPECT_TRUE((std::is_unbounded_array_v<int[]>));
		EXPECT_TRUE((std::is_scoped_enum_v<ScopedEnum>));
		EXPECT_FALSE((std::is_scoped_enum_v<UnscopedEnum>));
	}

	TEST(TypeTraits, ConstructionAssignmentAndDestructionTraits)
	{
		// These traits model object lifecycle capabilities and exception guarantees.
		EXPECT_TRUE((std::is_constructible_v<Derived>));
		EXPECT_TRUE((std::is_default_constructible_v<Derived>));
		EXPECT_TRUE((std::is_copy_constructible_v<Derived>));
		EXPECT_TRUE((std::is_move_constructible_v<Derived>));
		EXPECT_TRUE((std::is_assignable_v<int&, int>));
		EXPECT_TRUE((std::is_copy_assignable_v<Derived>));
		EXPECT_TRUE((std::is_move_assignable_v<Derived>));
		EXPECT_TRUE((std::is_destructible_v<Derived>));

		EXPECT_TRUE((std::is_trivially_constructible_v<TrivialType>));
		EXPECT_TRUE((std::is_trivially_copy_constructible_v<TrivialType>));
		EXPECT_TRUE((std::is_trivially_move_constructible_v<TrivialType>));
		EXPECT_TRUE((std::is_trivially_copy_assignable_v<TrivialType>));
		EXPECT_TRUE((std::is_trivially_move_assignable_v<TrivialType>));
		EXPECT_TRUE((std::is_trivially_destructible_v<TrivialType>));

		EXPECT_TRUE((std::is_nothrow_move_constructible_v<NothrowMove>));
		EXPECT_TRUE((std::is_nothrow_move_assignable_v<NothrowMove>));
		EXPECT_FALSE((std::is_nothrow_move_constructible_v<ThrowsMove>));
		EXPECT_FALSE((std::is_nothrow_move_assignable_v<ThrowsMove>));
	}

	TEST(TypeTraits, LayoutAndPolymorphismPropertyTraits)
	{
		// Property traits capture ABI/layout and OO behavior assumptions.
		EXPECT_TRUE((std::is_trivial_v<TrivialType>));
		EXPECT_TRUE((std::is_trivially_copyable_v<TrivialType>));
		EXPECT_TRUE((std::is_standard_layout_v<StandardLayoutType>));
		EXPECT_TRUE((std::is_empty_v<EmptyType>));
		EXPECT_FALSE((std::is_polymorphic_v<Derived>));
		EXPECT_FALSE((std::is_abstract_v<Derived>));
		EXPECT_TRUE((std::is_final_v<FinalType>));
		EXPECT_FALSE((std::is_aggregate_v<Derived>)); // has base class
		EXPECT_TRUE((std::has_virtual_destructor_v<std::exception>));
	}

	TEST(TypeTraits, RelationshipAndConvertibilityTraits)
	{
		// Relationship traits describe inheritance, convertibility, and type identity.
		EXPECT_TRUE((std::is_same_v<int, int>));
		EXPECT_TRUE((std::is_base_of_v<Base, Derived>));
		EXPECT_TRUE((std::is_convertible_v<Derived*, Base*>));
		EXPECT_TRUE((std::is_nothrow_convertible_v<int, long long>));
		EXPECT_TRUE((std::is_layout_compatible_v<StandardLayoutType, StandardLayoutType>));
		EXPECT_TRUE((std::is_pointer_interconvertible_base_of_v<Base, Derived>));
	}

	TEST(TypeTraits, InvocationTraitsAndReferenceUtilities)
	{
		// invoke_result/invoke and reference_wrapper related traits enable generic
		// callable dispatch and type deduction for call expressions.
		auto lambda = [](int v) noexcept { return v + 1; };
		using Result = std::invoke_result_t<decltype(lambda), int>;

		EXPECT_TRUE((std::is_same_v<Result, int>));
		EXPECT_TRUE((std::is_invocable_v<decltype(lambda), int>));
		EXPECT_TRUE((std::is_invocable_r_v<int, decltype(lambda), int>));
		EXPECT_TRUE((std::is_nothrow_invocable_v<decltype(lambda), int>));
		EXPECT_TRUE((std::is_nothrow_invocable_r_v<int, decltype(lambda), int>));
		EXPECT_FALSE((std::is_invocable_v<decltype(lambda), const char*>));
	}

	TEST(TypeTraits, TypeTransformationTraits)
	{
		// Transformation traits map input types to adjusted forms used in templates.
		EXPECT_TRUE((std::is_same_v<std::remove_const_t<const int>, int>));
		EXPECT_TRUE((std::is_same_v<std::remove_volatile_t<volatile int>, int>));
		EXPECT_TRUE((std::is_same_v<std::remove_cv_t<const volatile int>, int>));
		EXPECT_TRUE((std::is_same_v<std::add_const_t<int>, const int>));
		EXPECT_TRUE((std::is_same_v<std::add_volatile_t<int>, volatile int>));
		EXPECT_TRUE((std::is_same_v<std::add_cv_t<int>, const volatile int>));
		EXPECT_TRUE((std::is_same_v<std::remove_reference_t<int&>, int>));
		EXPECT_TRUE((std::is_same_v<std::add_lvalue_reference_t<int>, int&>));
		EXPECT_TRUE((std::is_same_v<std::add_rvalue_reference_t<int>, int&&>));
		EXPECT_TRUE((std::is_same_v<std::remove_pointer_t<int*>, int>));
		EXPECT_TRUE((std::is_same_v<std::add_pointer_t<int>, int*>));
		EXPECT_TRUE((std::is_same_v<std::make_signed_t<unsigned int>, int>));
		EXPECT_TRUE((std::is_same_v<std::make_unsigned_t<int>, unsigned int>));
		EXPECT_TRUE((std::is_same_v<std::remove_extent_t<int[4]>, int>));
		EXPECT_EQ((std::extent_v<int[2][3], 0>), 2u);
		EXPECT_EQ((std::extent_v<int[2][3], 1>), 3u);
		EXPECT_EQ((std::rank_v<int[2][3]>), 2u);
		EXPECT_TRUE((std::is_same_v<std::decay_t<int&>, int>));
		EXPECT_TRUE((std::is_same_v<std::remove_cvref_t<const int&>, int>));
		EXPECT_TRUE((std::is_same_v<std::type_identity_t<int>, int>));
	}

	TEST(TypeTraits, CommonTypeReferenceConditionalAndEnableIf)
	{
		// Meta-utility transformations for selecting and constraining types.
		EXPECT_TRUE((std::is_same_v<std::common_type_t<int, long>, long>));
		EXPECT_TRUE((std::is_same_v<std::common_reference_t<int&, const int&>, const int&>));
		EXPECT_TRUE((std::is_same_v<std::conditional_t<true, int, double>, int>));
		EXPECT_TRUE((std::is_same_v<std::enable_if_t<true, int>, int>));
	}

	TEST(TypeTraits, BooleanMetaLogicAndIntegralConstants)
	{
		// Integral constants and logical composition utilities are building blocks
		// for constexpr metaprogramming and SFINAE/concepts-era bridging.
		EXPECT_TRUE((std::true_type::value));
		EXPECT_FALSE((std::false_type::value));
		EXPECT_EQ((std::integral_constant<int, 7>::value), 7);
		EXPECT_TRUE((std::bool_constant<true>::value));
		EXPECT_TRUE((std::conjunction_v<std::true_type, std::true_type>));
		EXPECT_FALSE((std::conjunction_v<std::true_type, std::false_type>));
		EXPECT_TRUE((std::disjunction_v<std::false_type, std::true_type>));
		EXPECT_TRUE((std::negation_v<std::false_type>));
	}

	TEST(TypeTraits, RuntimeUtilitiesMoveForwardAsConstDeclval)
	{
		// Utility helpers in <type_traits> include move/forward/as_const/declval-
		// related type computations commonly used in generic forwarding code.
		int x = 5;
		int& lref = x;
		int&& rref = std::move(x);
		(void)lref;
		(void)rref;

		EXPECT_TRUE((std::is_same_v<decltype(std::as_const(x)), const int&>));
		EXPECT_TRUE((std::is_same_v<decltype(std::move(x)), int&&>));
		EXPECT_TRUE((std::is_same_v<decltype(std::forward<int&>(x)), int&>));
		EXPECT_TRUE((std::is_same_v<decltype(std::declval<int>()), int&&>));
	}

	TEST(TypeTraits, UnderlyingAndByteUtilityTraits)
	{
		// Enum underlying type extraction and to_underlying helpers are part of the
		// broader type utility ecosystem used with strongly typed enums.
		EXPECT_TRUE((std::is_same_v<std::underlying_type_t<ScopedEnum>, int>));
#ifdef __cpp_lib_to_underlying
		EXPECT_EQ(std::to_underlying(ScopedEnum::B), 2);
#endif
	}

}  // namespace
