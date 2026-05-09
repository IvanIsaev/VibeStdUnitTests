#include <gtest/gtest.h>

#include <bit>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace {

	struct Pair32
	{
		std::uint16_t a;
		std::uint16_t b;
	};

	TEST(BitHeader, EndianEnumeratorsAndNativeValueAreSensible)
	{
		// std::endian describes byte order categories. The named enumerators must be
		// distinct valid enum values, and native is one of little/big on mainstream
		// targets (or potentially neither for mixed-endian exotic systems).
		const auto little = std::endian::little;
		const auto big = std::endian::big;
		const auto native = std::endian::native;

		EXPECT_TRUE(little == std::endian::little);
		EXPECT_TRUE(big == std::endian::big);
		EXPECT_TRUE(native == std::endian::little ||
		            native == std::endian::big ||
		            native != std::endian::little);
	}

	TEST(BitHeader, BitCastReinterpretsObjectRepresentationSafely)
	{
		// std::bit_cast performs a bitwise-preserving cast between same-sized
		// trivially copyable types, avoiding UB from reinterpret_cast aliasing.
		static_assert(sizeof(std::uint32_t) == sizeof(float));
		const float pi = 3.1415926f;
		const std::uint32_t bits = std::bit_cast<std::uint32_t>(pi);
		const float roundTrip = std::bit_cast<float>(bits);
		EXPECT_EQ(roundTrip, pi);

		static_assert(sizeof(Pair32) == sizeof(std::uint32_t));
		const Pair32 pair{ 0x1122u, 0x3344u };
		const std::uint32_t packed = std::bit_cast<std::uint32_t>(pair);
		const Pair32 unpacked = std::bit_cast<Pair32>(packed);
		EXPECT_EQ(unpacked.a, pair.a);
		EXPECT_EQ(unpacked.b, pair.b);
	}

	TEST(BitHeader, PowerOfTwoPredicatesAndRoundingHelpers)
	{
		// has_single_bit checks if exactly one bit is set; bit_floor/bit_ceil find
		// nearest powers of two; bit_width returns required binary width.
		EXPECT_TRUE(std::has_single_bit(1u));
		EXPECT_TRUE(std::has_single_bit(8u));
		EXPECT_FALSE(std::has_single_bit(0u));
		EXPECT_FALSE(std::has_single_bit(10u));

		EXPECT_EQ(std::bit_floor(0u), 0u);
		EXPECT_EQ(std::bit_floor(1u), 1u);
		EXPECT_EQ(std::bit_floor(19u), 16u);

		EXPECT_EQ(std::bit_ceil(0u), 1u);
		EXPECT_EQ(std::bit_ceil(1u), 1u);
		EXPECT_EQ(std::bit_ceil(19u), 32u);

		EXPECT_EQ(std::bit_width(0u), 0u);
		EXPECT_EQ(std::bit_width(1u), 1u);
		EXPECT_EQ(std::bit_width(16u), 5u);
		EXPECT_EQ(std::bit_width(31u), 5u);
	}

	TEST(BitHeader, RotationsOperateWithinBitWidth)
	{
		// rotl/rotr perform circular shifts over the full unsigned type width.
		const std::uint8_t value = 0b10010001u;

		EXPECT_EQ(std::rotl<std::uint8_t>(value, 1), static_cast<std::uint8_t>(0b00100011u));
		EXPECT_EQ(std::rotr<std::uint8_t>(value, 1), static_cast<std::uint8_t>(0b11001000u));

		EXPECT_EQ(std::rotl<std::uint8_t>(value, 8), value);
		EXPECT_EQ(std::rotr<std::uint8_t>(value, 8), value);
	}

	TEST(BitHeader, LeadingAndTrailingBitCountingFunctions)
	{
		// countl_zero/countl_one and countr_zero/countr_one report contiguous
		// leading/trailing runs of zeros or ones in unsigned integer values.
		const std::uint8_t v1 = 0b00111100u;
		const std::uint8_t v2 = 0b11100011u;

		EXPECT_EQ(std::countl_zero(v1), 2);
		EXPECT_EQ(std::countl_one(v1), 0);
		EXPECT_EQ(std::countr_zero(v1), 2);
		EXPECT_EQ(std::countr_one(v1), 0);

		EXPECT_EQ(std::countl_one(v2), 3);
		EXPECT_EQ(std::countl_zero(v2), 0);
		EXPECT_EQ(std::countr_one(v2), 2);
		EXPECT_EQ(std::countr_zero(v2), 0);
	}

	TEST(BitHeader, PopcountCountsSetBits)
	{
		// popcount returns the number of set bits in an unsigned integer value.
		EXPECT_EQ(std::popcount(0u), 0);
		EXPECT_EQ(std::popcount(1u), 1);
		EXPECT_EQ(std::popcount(0b10101010u), 4);
		EXPECT_EQ(std::popcount(0xFFFFFFFFu), 32);
	}

	TEST(BitHeader, ByteswapWhenAvailableReversesByteOrder)
	{
		// std::byteswap (C++23) reverses byte order and is useful for endianness
		// conversions. Guarded for toolchains that may not yet provide it.
#ifdef __cpp_lib_byteswap
		EXPECT_GE(__cpp_lib_byteswap, 202110L);
		EXPECT_EQ(std::byteswap<std::uint16_t>(0x1234u), static_cast<std::uint16_t>(0x3412u));
		EXPECT_EQ(std::byteswap<std::uint32_t>(0x11223344u), 0x44332211u);
		EXPECT_EQ(std::byteswap<std::uint64_t>(0x0102030405060708ULL), 0x0807060504030201ULL);
#else
		GTEST_SKIP() << "std::byteswap is not available on this toolchain.";
#endif
	}

	TEST(BitHeader, UnsignedOnlyContractForBitOperationsViaTypeChecks)
	{
		// Most <bit> algorithms are specified for unsigned integer types. This test
		// verifies compile-time assumptions used throughout bit-manipulation code.
		EXPECT_TRUE((std::is_unsigned_v<std::uint8_t>));
		EXPECT_TRUE((std::is_unsigned_v<std::uint16_t>));
		EXPECT_TRUE((std::is_unsigned_v<std::uint32_t>));
		EXPECT_TRUE((std::is_unsigned_v<std::uint64_t>));
		EXPECT_EQ(std::numeric_limits<std::uint32_t>::digits, 32);
	}

}  // namespace
