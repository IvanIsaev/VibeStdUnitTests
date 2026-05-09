#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <type_traits>

namespace {

	TEST(CStdInt, ExactWidthSignedAndUnsignedTypeProperties)
	{
		// <cstdint> provides exact-width integer aliases (int8_t/int16_t/int32_t/
		// int64_t and unsigned counterparts) when the platform supports those exact
		// widths. On mainstream desktop targets these are available, so this test
		// verifies size and signedness contracts that make the types dependable for
		// binary protocols and file formats.
#ifdef INT8_MAX
		EXPECT_EQ(sizeof(std::int8_t), 1u);
		EXPECT_TRUE((std::is_signed_v<std::int8_t>));
		EXPECT_EQ(sizeof(std::uint8_t), 1u);
		EXPECT_TRUE((std::is_unsigned_v<std::uint8_t>));
#endif
#ifdef INT16_MAX
		EXPECT_EQ(sizeof(std::int16_t), 2u);
		EXPECT_TRUE((std::is_signed_v<std::int16_t>));
		EXPECT_EQ(sizeof(std::uint16_t), 2u);
		EXPECT_TRUE((std::is_unsigned_v<std::uint16_t>));
#endif
#ifdef INT32_MAX
		EXPECT_EQ(sizeof(std::int32_t), 4u);
		EXPECT_TRUE((std::is_signed_v<std::int32_t>));
		EXPECT_EQ(sizeof(std::uint32_t), 4u);
		EXPECT_TRUE((std::is_unsigned_v<std::uint32_t>));
#endif
#ifdef INT64_MAX
		EXPECT_EQ(sizeof(std::int64_t), 8u);
		EXPECT_TRUE((std::is_signed_v<std::int64_t>));
		EXPECT_EQ(sizeof(std::uint64_t), 8u);
		EXPECT_TRUE((std::is_unsigned_v<std::uint64_t>));
#endif
	}

	TEST(CStdInt, ExactWidthLimitMacrosMatchTypeTraits)
	{
		// The INTn_MIN/INTn_MAX and UINTn_MAX macros encode representable ranges for
		// exact-width aliases. This test cross-checks macro values against
		// std::numeric_limits to ensure macro and type interfaces stay coherent.
#ifdef INT8_MIN
		EXPECT_EQ(INT8_MIN, std::numeric_limits<std::int8_t>::min());
		EXPECT_EQ(INT8_MAX, std::numeric_limits<std::int8_t>::max());
		EXPECT_EQ(UINT8_MAX, std::numeric_limits<std::uint8_t>::max());
#endif
#ifdef INT16_MIN
		EXPECT_EQ(INT16_MIN, std::numeric_limits<std::int16_t>::min());
		EXPECT_EQ(INT16_MAX, std::numeric_limits<std::int16_t>::max());
		EXPECT_EQ(UINT16_MAX, std::numeric_limits<std::uint16_t>::max());
#endif
#ifdef INT32_MIN
		EXPECT_EQ(INT32_MIN, std::numeric_limits<std::int32_t>::min());
		EXPECT_EQ(INT32_MAX, std::numeric_limits<std::int32_t>::max());
		EXPECT_EQ(UINT32_MAX, std::numeric_limits<std::uint32_t>::max());
#endif
#ifdef INT64_MIN
		EXPECT_EQ(INT64_MIN, std::numeric_limits<std::int64_t>::min());
		EXPECT_EQ(INT64_MAX, std::numeric_limits<std::int64_t>::max());
		EXPECT_EQ(UINT64_MAX, std::numeric_limits<std::uint64_t>::max());
#endif
	}

	TEST(CStdInt, LeastWidthTypeFamiliesProvideMinimumBitCapacity)
	{
		// int_leastN_t/uint_leastN_t guarantee at least N value bits and are useful
		// when exact-width storage is unavailable. This test validates lower bounds
		// on width and confirms signed/unsigned polarity for each family.
		EXPECT_GE(sizeof(std::int_least8_t) * 8u, 8u);
		EXPECT_GE(sizeof(std::int_least16_t) * 8u, 16u);
		EXPECT_GE(sizeof(std::int_least32_t) * 8u, 32u);
		EXPECT_GE(sizeof(std::int_least64_t) * 8u, 64u);
		EXPECT_TRUE((std::is_signed_v<std::int_least8_t>));
		EXPECT_TRUE((std::is_signed_v<std::int_least16_t>));
		EXPECT_TRUE((std::is_signed_v<std::int_least32_t>));
		EXPECT_TRUE((std::is_signed_v<std::int_least64_t>));

		EXPECT_GE(sizeof(std::uint_least8_t) * 8u, 8u);
		EXPECT_GE(sizeof(std::uint_least16_t) * 8u, 16u);
		EXPECT_GE(sizeof(std::uint_least32_t) * 8u, 32u);
		EXPECT_GE(sizeof(std::uint_least64_t) * 8u, 64u);
		EXPECT_TRUE((std::is_unsigned_v<std::uint_least8_t>));
		EXPECT_TRUE((std::is_unsigned_v<std::uint_least16_t>));
		EXPECT_TRUE((std::is_unsigned_v<std::uint_least32_t>));
		EXPECT_TRUE((std::is_unsigned_v<std::uint_least64_t>));
	}

	TEST(CStdInt, FastWidthTypeFamiliesAreValidIntegerTypes)
	{
		// int_fastN_t/uint_fastN_t prioritize performance while preserving at least
		// N-bit capacity. These may be wider than N bits depending on ABI. The test
		// checks capacity and sign contracts without assuming a specific width model.
		EXPECT_GE(sizeof(std::int_fast8_t) * 8u, 8u);
		EXPECT_GE(sizeof(std::int_fast16_t) * 8u, 16u);
		EXPECT_GE(sizeof(std::int_fast32_t) * 8u, 32u);
		EXPECT_GE(sizeof(std::int_fast64_t) * 8u, 64u);
		EXPECT_TRUE((std::is_signed_v<std::int_fast8_t>));
		EXPECT_TRUE((std::is_signed_v<std::int_fast16_t>));
		EXPECT_TRUE((std::is_signed_v<std::int_fast32_t>));
		EXPECT_TRUE((std::is_signed_v<std::int_fast64_t>));

		EXPECT_GE(sizeof(std::uint_fast8_t) * 8u, 8u);
		EXPECT_GE(sizeof(std::uint_fast16_t) * 8u, 16u);
		EXPECT_GE(sizeof(std::uint_fast32_t) * 8u, 32u);
		EXPECT_GE(sizeof(std::uint_fast64_t) * 8u, 64u);
		EXPECT_TRUE((std::is_unsigned_v<std::uint_fast8_t>));
		EXPECT_TRUE((std::is_unsigned_v<std::uint_fast16_t>));
		EXPECT_TRUE((std::is_unsigned_v<std::uint_fast32_t>));
		EXPECT_TRUE((std::is_unsigned_v<std::uint_fast64_t>));
	}

	TEST(CStdInt, LeastAndFastLimitMacrosMatchTraits)
	{
		// Companion macros for least/fast families expose runtime-usable bounds
		// without templates. This test validates each macro against numeric_limits so
		// the macro and type views remain consistent for all width families.
		EXPECT_EQ(INT_LEAST8_MIN, std::numeric_limits<std::int_least8_t>::min());
		EXPECT_EQ(INT_LEAST8_MAX, std::numeric_limits<std::int_least8_t>::max());
		EXPECT_EQ(UINT_LEAST8_MAX, std::numeric_limits<std::uint_least8_t>::max());
		EXPECT_EQ(INT_LEAST16_MIN, std::numeric_limits<std::int_least16_t>::min());
		EXPECT_EQ(INT_LEAST16_MAX, std::numeric_limits<std::int_least16_t>::max());
		EXPECT_EQ(UINT_LEAST16_MAX, std::numeric_limits<std::uint_least16_t>::max());
		EXPECT_EQ(INT_LEAST32_MIN, std::numeric_limits<std::int_least32_t>::min());
		EXPECT_EQ(INT_LEAST32_MAX, std::numeric_limits<std::int_least32_t>::max());
		EXPECT_EQ(UINT_LEAST32_MAX, std::numeric_limits<std::uint_least32_t>::max());
		EXPECT_EQ(INT_LEAST64_MIN, std::numeric_limits<std::int_least64_t>::min());
		EXPECT_EQ(INT_LEAST64_MAX, std::numeric_limits<std::int_least64_t>::max());
		EXPECT_EQ(UINT_LEAST64_MAX, std::numeric_limits<std::uint_least64_t>::max());

		EXPECT_EQ(INT_FAST8_MIN, std::numeric_limits<std::int_fast8_t>::min());
		EXPECT_EQ(INT_FAST8_MAX, std::numeric_limits<std::int_fast8_t>::max());
		EXPECT_EQ(UINT_FAST8_MAX, std::numeric_limits<std::uint_fast8_t>::max());
		EXPECT_EQ(INT_FAST16_MIN, std::numeric_limits<std::int_fast16_t>::min());
		EXPECT_EQ(INT_FAST16_MAX, std::numeric_limits<std::int_fast16_t>::max());
		EXPECT_EQ(UINT_FAST16_MAX, std::numeric_limits<std::uint_fast16_t>::max());
		EXPECT_EQ(INT_FAST32_MIN, std::numeric_limits<std::int_fast32_t>::min());
		EXPECT_EQ(INT_FAST32_MAX, std::numeric_limits<std::int_fast32_t>::max());
		EXPECT_EQ(UINT_FAST32_MAX, std::numeric_limits<std::uint_fast32_t>::max());
		EXPECT_EQ(INT_FAST64_MIN, std::numeric_limits<std::int_fast64_t>::min());
		EXPECT_EQ(INT_FAST64_MAX, std::numeric_limits<std::int_fast64_t>::max());
		EXPECT_EQ(UINT_FAST64_MAX, std::numeric_limits<std::uint_fast64_t>::max());
	}

	TEST(CStdInt, MaxWidthAndPointerWidthTypesMatchTheirMacros)
	{
		// intmax_t/uintmax_t are the widest standard integer types and back APIs that
		// need architecture-independent "largest integer" handling. intptr_t/uintptr_t
		// are integer types capable of round-tripping pointer values when provided.
		// This test checks signedness and macro/type consistency for these families.
		EXPECT_TRUE((std::is_signed_v<std::intmax_t>));
		EXPECT_TRUE((std::is_unsigned_v<std::uintmax_t>));
		EXPECT_EQ(INTMAX_MIN, std::numeric_limits<std::intmax_t>::min());
		EXPECT_EQ(INTMAX_MAX, std::numeric_limits<std::intmax_t>::max());
		EXPECT_EQ(UINTMAX_MAX, std::numeric_limits<std::uintmax_t>::max());

#ifdef INTPTR_MIN
		EXPECT_TRUE((std::is_signed_v<std::intptr_t>));
		EXPECT_EQ(INTPTR_MIN, std::numeric_limits<std::intptr_t>::min());
		EXPECT_EQ(INTPTR_MAX, std::numeric_limits<std::intptr_t>::max());
#endif
#ifdef UINTPTR_MAX
		EXPECT_TRUE((std::is_unsigned_v<std::uintptr_t>));
		EXPECT_EQ(UINTPTR_MAX, std::numeric_limits<std::uintptr_t>::max());
#endif
	}

	TEST(CStdInt, IntegerConstantMacrosPreserveRequestedWidthFamilies)
	{
		// INTn_C/UINTn_C and INTMAX_C/UINTMAX_C construct integer literals with at
		// least the target width family, preventing accidental narrowing from default
		// literal typing rules. We validate result types via decltype and values.
		EXPECT_TRUE((std::is_same_v<decltype(INT8_C(12)), std::int_least8_t>));
		EXPECT_TRUE((std::is_same_v<decltype(UINT8_C(12)), std::uint_least8_t>));
		EXPECT_TRUE((std::is_same_v<decltype(INT16_C(300)), std::int_least16_t>));
		EXPECT_TRUE((std::is_same_v<decltype(UINT16_C(300)), std::uint_least16_t>));
		EXPECT_TRUE((std::is_same_v<decltype(INT32_C(70000)), std::int_least32_t>));
		EXPECT_TRUE((std::is_same_v<decltype(UINT32_C(70000)), std::uint_least32_t>));
		EXPECT_TRUE((std::is_same_v<decltype(INT64_C(7000000000)), std::int_least64_t>));
		EXPECT_TRUE((std::is_same_v<decltype(UINT64_C(7000000000)), std::uint_least64_t>));
		EXPECT_TRUE((std::is_same_v<decltype(INTMAX_C(42)), std::intmax_t>));
		EXPECT_TRUE((std::is_same_v<decltype(UINTMAX_C(42)), std::uintmax_t>));

		EXPECT_EQ(INT8_C(12), 12);
		EXPECT_EQ(UINT8_C(12), 12u);
		EXPECT_EQ(INTMAX_C(42), 42);
		EXPECT_EQ(UINTMAX_C(42), 42u);
	}

}  // namespace
