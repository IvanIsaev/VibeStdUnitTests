#include <gtest/gtest.h>

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <iterator>

namespace {

	TEST(CStdLib, AtoiAtofAtol)
	{
		// atoi/atof/atol convert leading numeric text to int/double/long respectively.
		// This test validates straightforward decimal parsing and verifies that
		// integer conversions stop at the first non-numeric character while keeping
		// the successfully parsed prefix.
		EXPECT_EQ(std::atoi("42"), 42);
		EXPECT_EQ(std::atoi("-17xyz"), -17);
		EXPECT_DOUBLE_EQ(std::atof("3.5"), 3.5);
		EXPECT_EQ(std::atol("12345"), 12345L);
	}

	TEST(CStdLib, StrtolAndStrtoulWithEndPointer)
	{
		// strtol/strtoul provide robust conversion with base selection and an end
		// pointer that reports where parsing stopped. This test checks base-16 parsing,
		// confirms the trailing suffix is not consumed, and verifies unsigned parsing
		// from an octal-like representation when base is auto-detected.
		char* end = nullptr;
		const long hexValue = std::strtol("7frest", &end, 16);
		ASSERT_NE(end, nullptr);
		EXPECT_EQ(hexValue, 127L);
		EXPECT_STREQ(end, "rest");

		end = nullptr;
		const unsigned long octValue = std::strtoul("077", &end, 0);
		ASSERT_NE(end, nullptr);
		EXPECT_EQ(octValue, 63UL);
		EXPECT_STREQ(end, "");
	}

	TEST(CStdLib, StrtodReportsConsumedSuffix)
	{
		// strtod converts floating-point text and reports the first unparsed
		// character via end pointer. This is useful when a numeric prefix is followed
		// by units (e.g. "12.5ms"), and we validate that both value and suffix split
		// are correct.
		char* end = nullptr;
		const double value = std::strtod("12.5ms", &end);
		ASSERT_NE(end, nullptr);
		EXPECT_DOUBLE_EQ(value, 12.5);
		EXPECT_STREQ(end, "ms");
	}

	TEST(CStdLib, StrtolOverflowSetsErrno)
	{
		// strtol signals out-of-range conversions by clamping to LONG_MAX/LONG_MIN
		// and setting errno to ERANGE. The exact overflow threshold is platform-
		// dependent, so this test uses a very large literal to guarantee overflow
		// and verifies both the clamped result and errno signaling behavior.
		errno = 0;
		const long value = std::strtol("999999999999999999999999999999", nullptr, 10);
		EXPECT_EQ(value, LONG_MAX);
		EXPECT_EQ(errno, ERANGE);
	}

	TEST(CStdLib, AbsoluteValueFunctions)
	{
		// abs/labs/llabs compute the magnitude of signed integers for different
		// widths. These functions are useful when exact integer type preservation is
		// needed, and this test validates representative positive and negative inputs.
		EXPECT_EQ(std::abs(-11), 11);
		EXPECT_EQ(std::labs(-123456L), 123456L);
		EXPECT_EQ(std::llabs(-1234567890123LL), 1234567890123LL);
	}

	TEST(CStdLib, QuotientAndRemainderStructures)
	{
		// div/ldiv/lldiv perform integer division while returning quotient and
		// remainder together in POD structures. This test confirms both fields across
		// int, long, and long long variants to ensure consistent arithmetic behavior.
		const std::div_t d = std::div(17, 5);
		EXPECT_EQ(d.quot, 3);
		EXPECT_EQ(d.rem, 2);

		const std::ldiv_t ld = std::ldiv(100L, 9L);
		EXPECT_EQ(ld.quot, 11L);
		EXPECT_EQ(ld.rem, 1L);

		const std::lldiv_t lld = std::lldiv(1000LL, 64LL);
		EXPECT_EQ(lld.quot, 15LL);
		EXPECT_EQ(lld.rem, 40LL);
	}

	TEST(CStdLib, RandIsSeedDeterministic)
	{
		// srand initializes the pseudo-random sequence consumed by rand. Reseeding
		// with the same value must restart the same deterministic sequence, which is
		// critical for reproducible tests and simulations.
		std::srand(12345);
		const int first = std::rand();
		std::srand(12345);
		const int second = std::rand();

		EXPECT_EQ(first, second);
		EXPECT_GE(first, 0);
		EXPECT_LE(first, RAND_MAX);
	}

	TEST(CStdLib, MallocCallocReallocFree)
	{
		// malloc/calloc/realloc/free provide manual storage management. This test
		// validates that calloc returns zero-initialized bytes, realloc preserves the
		// existing prefix during growth, and memory can be safely released.
		unsigned char* bytes = static_cast<unsigned char*>(std::calloc(4, sizeof(unsigned char)));
		ASSERT_NE(bytes, nullptr);
		for (int i = 0; i < 4; ++i)
		{
			EXPECT_EQ(bytes[i], 0u);
		}

		bytes[0] = 7;
		bytes[1] = 9;
		unsigned char* grown = static_cast<unsigned char*>(std::realloc(bytes, 8));
		ASSERT_NE(grown, nullptr);
		EXPECT_EQ(grown[0], 7u);
		EXPECT_EQ(grown[1], 9u);

		std::free(grown);
	}

	TEST(CStdLib, QsortOrdersArray)
	{
		// qsort performs C-style in-place sorting via a comparator callback and raw
		// byte manipulation. This test confirms ascending order for an integer array
		// and validates interoperability with C++ code that still relies on qsort.
		auto compareInts = [](const void* lhs, const void* rhs) -> int
		{
			const int left = *static_cast<const int*>(lhs);
			const int right = *static_cast<const int*>(rhs);
			if (left < right) return -1;
			if (left > right) return 1;
			return 0;
		};

		int values[] = { 5, 1, 4, 2, 3 };
		std::qsort(values, std::size(values), sizeof(int), compareInts);
		EXPECT_EQ(values[0], 1);
		EXPECT_EQ(values[1], 2);
		EXPECT_EQ(values[2], 3);
		EXPECT_EQ(values[3], 4);
		EXPECT_EQ(values[4], 5);
	}

	TEST(CStdLib, BsearchFindsAndMisses)
	{
		// bsearch performs binary search over a sorted C array using a comparator.
		// This test verifies both successful lookup (non-null result with expected
		// value) and unsuccessful lookup (null pointer), covering both result paths.
		auto compareInts = [](const void* key, const void* elem) -> int
		{
			const int k = *static_cast<const int*>(key);
			const int e = *static_cast<const int*>(elem);
			if (k < e) return -1;
			if (k > e) return 1;
			return 0;
		};

		const int sorted[] = { 2, 4, 6, 8, 10 };
		const int keyHit = 6;
		const int* hit = static_cast<const int*>(std::bsearch(
			&keyHit, sorted, std::size(sorted), sizeof(int), compareInts));
		ASSERT_NE(hit, nullptr);
		EXPECT_EQ(*hit, 6);

		const int keyMiss = 7;
		const int* miss = static_cast<const int*>(std::bsearch(
			&keyMiss, sorted, std::size(sorted), sizeof(int), compareInts));
		EXPECT_EQ(miss, nullptr);
	}

	TEST(CStdLib, GetenvAndProcessConstants)
	{
		// getenv queries process environment variables and returns nullptr when a
		// variable does not exist. This test validates one known-present variable on
		// Windows ("PATH"), checks that a nonsense key is absent, and asserts basic
		// process-related constants exposed by <cstdlib> are sensible.
		const char* pathValue = std::getenv("PATH");
		ASSERT_NE(pathValue, nullptr);
		EXPECT_GT(std::strlen(pathValue), 0u);

		const char* missing = std::getenv("THIS_VARIABLE_SHOULD_NOT_EXIST_0E6C3E8C");
		EXPECT_EQ(missing, nullptr);

		EXPECT_GE(MB_CUR_MAX, 1);
		EXPECT_EQ(EXIT_SUCCESS, 0);
		EXPECT_NE(EXIT_FAILURE, EXIT_SUCCESS);
	}

}  // namespace
