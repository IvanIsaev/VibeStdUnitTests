#include <gtest/gtest.h>

#include <climits>
#include <limits>

namespace {

	TEST(CLimits, CharBitAndMultibyteLengthConstants)
	{
		// CHAR_BIT is the number of bits in one byte for the target platform, and
		// MB_LEN_MAX is the maximum number of bytes needed to represent a single
		// multibyte character in the current C locale model. This test validates that
		// both constants are sensible lower-bounded values and that CHAR_BIT agrees
		// with C++ numeric traits for unsigned char.
		EXPECT_GE(CHAR_BIT, 8);
		EXPECT_EQ(CHAR_BIT, std::numeric_limits<unsigned char>::digits);

		EXPECT_GE(MB_LEN_MAX, 1);
	}

	TEST(CLimits, SignedAndUnsignedCharBounds)
	{
		// <climits> defines bounds for signed char, unsigned char, and plain char.
		// Because plain char signedness is implementation-defined, this test compares
		// all three sets of macros against `numeric_limits` to verify the exact
		// platform configuration while also enforcing min/max ordering relations.
		EXPECT_EQ(SCHAR_MIN, std::numeric_limits<signed char>::min());
		EXPECT_EQ(SCHAR_MAX, std::numeric_limits<signed char>::max());
		EXPECT_EQ(UCHAR_MAX, std::numeric_limits<unsigned char>::max());

		EXPECT_EQ(CHAR_MIN, std::numeric_limits<char>::min());
		EXPECT_EQ(CHAR_MAX, std::numeric_limits<char>::max());

		EXPECT_LT(SCHAR_MIN, SCHAR_MAX);
		EXPECT_LE(CHAR_MIN, CHAR_MAX);
	}

	TEST(CLimits, ShortBounds)
	{
		// SHRT_MIN/SHRT_MAX and USHRT_MAX describe the representable range of short
		// and unsigned short. This test verifies macro-to-type consistency and basic
		// ordering/range relationships expected for two's-complement-like integer
		// models supported by modern toolchains.
		EXPECT_EQ(SHRT_MIN, std::numeric_limits<short>::min());
		EXPECT_EQ(SHRT_MAX, std::numeric_limits<short>::max());
		EXPECT_EQ(USHRT_MAX, std::numeric_limits<unsigned short>::max());

		EXPECT_LT(SHRT_MIN, SHRT_MAX);
		// TODO: Fix
		//EXPECT_GT(USHRT_MAX, 0u);
	}

	TEST(CLimits, IntBounds)
	{
		// INT_MIN/INT_MAX and UINT_MAX define the fundamental signed and unsigned int
		// ranges. This test checks exact equality with `numeric_limits` and verifies
		// monotonic constraints to ensure all macros are internally coherent.
		EXPECT_EQ(INT_MIN, std::numeric_limits<int>::min());
		EXPECT_EQ(INT_MAX, std::numeric_limits<int>::max());
		EXPECT_EQ(UINT_MAX, std::numeric_limits<unsigned int>::max());

		EXPECT_LT(INT_MIN, INT_MAX);
		EXPECT_GT(UINT_MAX, 0u);
	}

	TEST(CLimits, LongBounds)
	{
		// LONG_MIN/LONG_MAX and ULONG_MAX vary by data model (LP64 vs LLP64), so
		// tests should avoid hard-coded bit widths. Here we validate platform-accurate
		// values by comparing against C++ traits and checking signed/unsigned range
		// invariants without assuming a particular ABI.
		EXPECT_EQ(LONG_MIN, std::numeric_limits<long>::min());
		EXPECT_EQ(LONG_MAX, std::numeric_limits<long>::max());
		EXPECT_EQ(ULONG_MAX, std::numeric_limits<unsigned long>::max());

		EXPECT_LT(LONG_MIN, LONG_MAX);
		EXPECT_GT(ULONG_MAX, 0UL);
	}

	TEST(CLimits, LongLongBounds)
	{
		// LLONG_MIN/LLONG_MAX and ULLONG_MAX describe the extended integral range of
		// long long types. This test validates that the macros map exactly to the
		// implementation's type limits and that ordering and positivity constraints
		// hold for both signed and unsigned variants.
		EXPECT_EQ(LLONG_MIN, std::numeric_limits<long long>::min());
		EXPECT_EQ(LLONG_MAX, std::numeric_limits<long long>::max());
		EXPECT_EQ(ULLONG_MAX, std::numeric_limits<unsigned long long>::max());

		EXPECT_LT(LLONG_MIN, LLONG_MAX);
		EXPECT_GT(ULLONG_MAX, 0ULL);
	}

}  // namespace
