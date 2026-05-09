#include <gtest/gtest.h>

#include <exception>
#include <string_view>
#include <type_traits>
#include <typeinfo>

namespace {

	struct PolyBase
	{
		virtual ~PolyBase() = default;
	};

	struct PolyDerived : PolyBase {};

	struct AnotherDerived : PolyBase {};

	TEST(TypeInfo, TypeInfoObjectModelsRuntimeTypeIdentity)
	{
		// std::type_info objects are produced by typeid expressions and model type
		// identity at runtime. Same-type expressions must yield equal type_info
		// identities, while different types must produce non-equal identities.
		const std::type_info& intInfoA = typeid(int);
		const std::type_info& intInfoB = typeid(int);
		const std::type_info& doubleInfo = typeid(double);

		EXPECT_EQ(intInfoA, intInfoB);
		EXPECT_NE(intInfoA, doubleInfo);
	}

	TEST(TypeInfo, TypeInfoNameAndHashCodeAreStableForSameType)
	{
		// name() and hash_code() are implementation-defined representations, but they
		// must be self-consistent for the same type within a run. This test checks
		// non-empty naming and stable hash/name for repeated typeid(int).
		const std::type_info& first = typeid(int);
		const std::type_info& second = typeid(int);
		const std::type_info& third = typeid(long);

		EXPECT_NE(std::string_view(first.name()).size(), 0u);
		EXPECT_EQ(first.hash_code(), second.hash_code());
		EXPECT_EQ(std::string_view(first.name()), std::string_view(second.name()));
		EXPECT_NE(first.hash_code(), third.hash_code());
	}

	TEST(TypeInfo, TypeInfoIsNotCopyConstructibleOrAssignable)
	{
		// std::type_info intentionally uses reference-like semantics and cannot be
		// copied or assigned by user code. This encourages use through references
		// (usually from typeid) and wrappers such as std::type_index.
		EXPECT_FALSE((std::is_copy_constructible_v<std::type_info>));
		EXPECT_FALSE((std::is_copy_assignable_v<std::type_info>));
	}

	TEST(TypeInfo, TypeidOnPolymorphicReferenceReportsDynamicType)
	{
		// Applying typeid to a polymorphic glvalue yields dynamic type information.
		// This is central to RTTI: a base reference to derived object should report
		// the derived type_info rather than static base type_info.
		PolyDerived derived;
		PolyBase& asBase = derived;

		EXPECT_EQ(typeid(asBase), typeid(PolyDerived));
		EXPECT_NE(typeid(asBase), typeid(PolyBase));
	}

	TEST(TypeInfo, TypeidOnNullPolymorphicPointerDereferenceThrowsBadTypeid)
	{
		// typeid(*ptr) where ptr is a null pointer to polymorphic type throws
		// std::bad_typeid, because runtime type discovery requires a valid object.
		PolyBase* nullBase = nullptr;
		EXPECT_THROW((void)typeid(*nullBase), std::bad_typeid);
	}

	TEST(TypeInfo, BadTypeidAndBadCastInheritFromStdException)
	{
		// <typeinfo> defines std::bad_typeid and std::bad_cast as RTTI-related
		// exception types. Both participate in the standard exception hierarchy and
		// provide diagnostic what() messages.
		EXPECT_TRUE((std::is_base_of_v<std::exception, std::bad_typeid>));
		EXPECT_TRUE((std::is_base_of_v<std::exception, std::bad_cast>));

		const std::bad_typeid badTypeId;
		const std::bad_cast badCast;
		EXPECT_NE(*badTypeId.what(), '\0');
		EXPECT_NE(*badCast.what(), '\0');
	}

	TEST(TypeInfo, FailedReferenceDynamicCastThrowsBadCast)
	{
		// A failed dynamic_cast to reference type throws std::bad_cast. This tests
		// runtime cast failure behavior and ensures the dedicated RTTI exception path
		// from <typeinfo> is observed.
		AnotherDerived obj;
		PolyBase& baseRef = obj;

		EXPECT_THROW((void)dynamic_cast<PolyDerived&>(baseRef), std::bad_cast);
	}

}  // namespace
