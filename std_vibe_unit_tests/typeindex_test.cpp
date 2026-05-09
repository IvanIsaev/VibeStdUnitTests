#include <gtest/gtest.h>

#include <string_view>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>

namespace {

	struct BaseType
	{
		virtual ~BaseType() = default;
	};

	struct DerivedType : BaseType {};

	TEST(TypeIndex, ConstructionFromTypeInfoProvidesStableIdentityToken)
	{
		// std::type_index wraps std::type_info references into a copyable/comparable
		// value object. Constructing from typeid(T) should create a stable token that
		// compares equal for the same type and different for distinct types.
		const std::type_index intA(typeid(int));
		const std::type_index intB(typeid(int));
		const std::type_index dbl(typeid(double));

		EXPECT_EQ(intA, intB);
		EXPECT_NE(intA, dbl);
	}

	TEST(TypeIndex, NameAndHashCodeMirrorUnderlyingTypeInfo)
	{
		// type_index exposes name() and hash_code() from the wrapped type_info. This
		// test verifies forwarding behavior for same and different wrapped types.
		const std::type_index intIndex(typeid(int));
		const std::type_index intAgain(typeid(int));
		const std::type_index longIndex(typeid(long));

		EXPECT_EQ(intIndex.hash_code(), intAgain.hash_code());
		EXPECT_EQ(intIndex.hash_code(), typeid(int).hash_code());
		EXPECT_EQ(std::string_view(intIndex.name()), std::string_view(typeid(int).name()));
		EXPECT_NE(std::string_view(intIndex.name()), std::string_view(longIndex.name()));
	}

	TEST(TypeIndex, ComparisonOperationsProvideStrictWeakOrdering)
	{
		// <typeindex> provides relational comparison operators (and in modern C++
		// also three-way comparison support) so type_index can be used in ordered
		// containers and sorted algorithms. We validate ordering consistency.
		const std::type_index a(typeid(char));
		const std::type_index b(typeid(int));
		const std::type_index c(typeid(double));

		// Equality relation should be reflexive.
		EXPECT_TRUE(a == a);
		EXPECT_FALSE(a != a);

		// At least one strict ordering relation must hold between distinct tokens.
		const bool abOrdered = (a < b) || (b < a);
		const bool bcOrdered = (b < c) || (c < b);
		EXPECT_TRUE(abOrdered);
		EXPECT_TRUE(bcOrdered);

		// If a < b then b should not be < a (antisymmetry for strict ordering).
		if (a < b)
		{
			EXPECT_FALSE(b < a);
		}
		if (b < c)
		{
			EXPECT_FALSE(c < b);
		}
	}

	TEST(TypeIndex, WorksAsKeyInUnorderedMapViaStdHashSpecialization)
	{
		// std::hash<std::type_index> is provided by <typeindex>, enabling direct use
		// as a key in unordered associative containers. This test verifies insertion
		// and lookup by dynamic/static type tokens.
		std::unordered_map<std::type_index, int> typeToId;
		typeToId.emplace(std::type_index(typeid(int)), 10);
		typeToId.emplace(std::type_index(typeid(double)), 20);

		EXPECT_EQ(typeToId.at(std::type_index(typeid(int))), 10);
		EXPECT_EQ(typeToId.at(std::type_index(typeid(double))), 20);
		EXPECT_EQ(typeToId.count(std::type_index(typeid(long))), 0u);
	}

	TEST(TypeIndex, PolymorphicTypeidProducesDynamicTypeToken)
	{
		// When typeid is applied to a polymorphic glvalue, it yields the dynamic
		// type's type_info. Wrapping that result into type_index should therefore
		// identify the most-derived runtime type, not merely the static base type.
		DerivedType derived;
		BaseType& baseRef = derived;

		const std::type_index dynamicIndex(typeid(baseRef));
		const std::type_index staticBaseIndex(typeid(BaseType));
		const std::type_index derivedIndex(typeid(DerivedType));

		EXPECT_EQ(dynamicIndex, derivedIndex);
		EXPECT_NE(dynamicIndex, staticBaseIndex);
	}

	TEST(TypeIndex, HashFunctorMatchesTypeIndexHashCodeContract)
	{
		// hash<type_index>{}(x) is specified to produce a hash value consistent with
		// x.hash_code(), which enables coherent behavior across hashing APIs.
		const std::type_index index(typeid(unsigned long long));
		const std::size_t fromMember = index.hash_code();
		const std::size_t fromFunctor = std::hash<std::type_index>{}(index);

		EXPECT_EQ(fromFunctor, fromMember);
	}

}  // namespace
