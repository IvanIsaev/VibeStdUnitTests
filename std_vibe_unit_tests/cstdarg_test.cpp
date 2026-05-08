#include <gtest/gtest.h>

#include <cstdarg>

namespace {

int SumIntsWithVarArgs(int count, ...)
{
	// va_list is the cursor-like object that tracks traversal state over a
	// variadic argument sequence. va_start initializes it using the last named
	// parameter (`count`), after which va_arg can read each argument in order.
	va_list args;
	va_start(args, count);

	int total = 0;
	for (int i = 0; i < count; ++i)
	{
		total += va_arg(args, int);
	}

	// va_end must be called before leaving scope to complete traversal and allow
	// implementations that require cleanup for the internal va_list state.
	va_end(args);
	return total;
}

int SumFirstNFromCopiedList(int count, va_list source)
{
	// va_copy duplicates traversal state from an existing va_list so both lists
	// can be consumed independently. This is required because plain assignment is
	// not portable for va_list on all ABIs/platforms.
	va_list copied;
	va_copy(copied, source);

	int total = 0;
	for (int i = 0; i < count; ++i)
	{
		total += va_arg(copied, int);
	}

	va_end(copied);
	return total;
}

int CompareOriginalAndCopyConsumption(int count, ...)
{
	// This helper demonstrates that the original va_list and a copied va_list can
	// be advanced separately and still read the same sequence from their own
	// cursors. We sum with the copy, then sum with the original, and return the
	// difference (expected to be zero when both consumed identically).
	va_list original;
	va_start(original, count);

	const int sumFromCopy = SumFirstNFromCopiedList(count, original);

	int sumFromOriginal = 0;
	for (int i = 0; i < count; ++i)
	{
		sumFromOriginal += va_arg(original, int);
	}

	va_end(original);
	return sumFromOriginal - sumFromCopy;
}

TEST(CStdArg, VaStartVaArgVaEndIterateVariadicInts)
{
	// Covers va_start + va_arg + va_end together in a typical reduction pattern:
	// initialize traversal, read each typed argument in order, and finalize.
	EXPECT_EQ(SumIntsWithVarArgs(0), 0);
	EXPECT_EQ(SumIntsWithVarArgs(1, 5), 5);
	EXPECT_EQ(SumIntsWithVarArgs(4, 1, 2, 3, 4), 10);
	EXPECT_EQ(SumIntsWithVarArgs(5, -2, 7, 0, -1, 6), 10);
}

TEST(CStdArg, VaCopyCreatesIndependentTraversalState)
{
	// Covers va_copy explicitly by proving copied and original lists can each
	// consume the same arguments independently without interfering.
	EXPECT_EQ(CompareOriginalAndCopyConsumption(3, 10, 20, 30), 0);
	EXPECT_EQ(CompareOriginalAndCopyConsumption(5, 1, 1, 2, 3, 5), 0);
}

TEST(CStdArg, VaListTypeIsUsableInFunctionInterfaces)
{
	// Covers va_list as a first-class interface type by passing it through helper
	// functions (`CompareOriginalAndCopyConsumption` -> `SumFirstNFromCopiedList`)
	// and validating behavior through an externally observable result.
	EXPECT_EQ(CompareOriginalAndCopyConsumption(4, 4, 3, 2, 1), 0);
}

}  // namespace
