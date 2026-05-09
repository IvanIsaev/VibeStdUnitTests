#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace {

	TEST(CStdDef, NullMacroActsAsNullPointerConstant)
	{
		// NULL from <cstddef> is required to be a null pointer constant in C++.
		// Implementations may define it as literal 0 or nullptr, but either form
		// must be safely assignable to object pointers and compare equal to nullptr.
		int* intPointer = NULL;
		void* voidPointer = NULL;

		EXPECT_EQ(intPointer, nullptr);
		EXPECT_EQ(voidPointer, nullptr);
	}

	struct OffsetLayout
	{
		char marker;
		int payload;
		double value;
	};

	TEST(CStdDef, OffsetofReturnsMemberByteOffsets)
	{
		// offsetof computes the byte displacement of a standard-layout member from
		// the beginning of its enclosing object. This test validates monotonic member
		// ordering and cross-checks one offset with a runtime pointer subtraction.
		static_assert(std::is_standard_layout_v<OffsetLayout>);

		constexpr std::size_t markerOffset = offsetof(OffsetLayout, marker);
		constexpr std::size_t payloadOffset = offsetof(OffsetLayout, payload);
		constexpr std::size_t valueOffset = offsetof(OffsetLayout, value);

		EXPECT_EQ(markerOffset, 0u);
		EXPECT_LT(markerOffset, payloadOffset);
		EXPECT_LT(payloadOffset, valueOffset);

		OffsetLayout sample{};
		const auto* base = reinterpret_cast<const unsigned char*>(&sample);
		const auto* payloadAddress = reinterpret_cast<const unsigned char*>(&sample.payload);
		const std::size_t runtimeOffset = static_cast<std::size_t>(payloadAddress - base);
		EXPECT_EQ(payloadOffset, runtimeOffset);
	}

	TEST(CStdDef, PtrdiffTypeRepresentsPointerSubtraction)
	{
		// ptrdiff_t is the signed integer type produced by subtracting two pointers
		// into the same array. This test verifies both signedness and exact distance
		// semantics for a simple contiguous array.
		std::array<long, 6> values = { 10, 20, 30, 40, 50, 60 };
		const std::ptrdiff_t distance = &values[5] - &values[1];

		EXPECT_TRUE((std::is_signed_v<std::ptrdiff_t>));
		EXPECT_EQ(distance, 4);
	}

	TEST(CStdDef, SizeTypeCanRepresentObjectSizes)
	{
		// size_t is the canonical unsigned type for object sizes and counts produced
		// by sizeof. This test checks type-level properties and validates that a
		// compile-time object size can be carried in size_t without narrowing.
		EXPECT_TRUE((std::is_unsigned_v<std::size_t>));

		constexpr std::size_t objectSize = sizeof(std::array<int, 8>);
		EXPECT_GE(objectSize, sizeof(int) * 8u);
	}

	TEST(CStdDef, MaxAlignTypeProvidesStrongAlignment)
	{
		// max_align_t has an alignment requirement at least as strict as any scalar
		// fundamental type that might appear in ordinary storage. This makes it a
		// safe anchor for generic aligned storage and allocator internals.
		EXPECT_GE(alignof(std::max_align_t), alignof(long double));
		EXPECT_GE(alignof(std::max_align_t), alignof(long long));
		EXPECT_GE(alignof(std::max_align_t), alignof(void*));
	}

	TEST(CStdDef, NullptrTypeMatchesDecltypeNullptr)
	{
		// nullptr_t is the dedicated type of the nullptr literal. Verifying this
		// identity ensures overload resolution and null-pointer APIs can rely on the
		// exact type exposed by <cstddef>.
		EXPECT_TRUE((std::is_same_v<std::nullptr_t, decltype(nullptr)>));
	}

	TEST(CStdDef, ByteSupportsBitwiseOperationsAndIntegerConversion)
	{
		// std::byte is an enum-like byte container intended for raw memory and
		// bit-manipulation. It intentionally avoids arithmetic, but supports bitwise
		// operators and conversion through std::to_integer.
		std::byte value{ 0x0F };
		value <<= 1;
		EXPECT_EQ(std::to_integer<unsigned int>(value), 0x1Eu);

		const std::byte mask{ 0x30 };
		const std::byte combined = value | mask;
		EXPECT_EQ(std::to_integer<unsigned int>(combined), 0x3Eu);
	}

}  // namespace
