#include <gtest/gtest.h>

#include <bitset>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

	TEST(Bitset, ConstructionFromDefaultUnsignedAndStringForms)
	{
		// std::bitset can be default-constructed (all zeros), constructed from an
		// unsigned integer value, or parsed from a textual bit representation.
		std::bitset<8> empty;
		EXPECT_TRUE(empty.none());
		EXPECT_EQ(empty.count(), 0u);

		std::bitset<8> fromValue(0b10100110u);
		EXPECT_TRUE(fromValue.test(1));
		EXPECT_TRUE(fromValue.test(2));
		EXPECT_TRUE(fromValue.test(5));
		EXPECT_TRUE(fromValue.test(7));

		std::bitset<8> fromString(std::string("11010010"));
		EXPECT_EQ(fromString.to_string(), "11010010");
	}

	TEST(Bitset, SizeAndElementAccessOperators)
	{
		// size() reports template bit width. operator[] provides proxy write access
		// on mutable bitsets and value read access on const bitsets.
		std::bitset<16> bits;
		EXPECT_EQ(bits.size(), 16u);

		bits[3] = true;
		bits[4] = true;
		EXPECT_TRUE(bits[3]);
		EXPECT_TRUE(bits[4]);
		EXPECT_FALSE(bits[5]);

		const std::bitset<16>& constBits = bits;
		EXPECT_TRUE(constBits[3]);
		EXPECT_FALSE(constBits[10]);
	}

	TEST(Bitset, TestMethodAndRangeCheckingBehavior)
	{
		// test(pos) checks one bit and throws std::out_of_range for invalid indexes.
		std::bitset<4> bits(std::string("0101"));
		EXPECT_TRUE(bits.test(0));
		EXPECT_FALSE(bits.test(1));
		EXPECT_TRUE(bits.test(2));
		EXPECT_FALSE(bits.test(3));

		EXPECT_THROW((void)bits.test(4), std::out_of_range);
	}

	TEST(Bitset, SetResetFlipModifiersAndWholeBitsetOperations)
	{
		// set/reset/flip support both per-bit and whole-bitset operations, which are
		// core mutators for bit-manipulation workflows.
		std::bitset<8> bits;

		bits.set(0);
		bits.set(7);
		EXPECT_EQ(bits.to_string(), "10000001");

		bits.reset(0);
		EXPECT_EQ(bits.to_string(), "10000000");

		bits.flip(6);
		EXPECT_EQ(bits.to_string(), "11000000");

		bits.set();
		EXPECT_TRUE(bits.all());
		EXPECT_EQ(bits.count(), 8u);

		bits.reset();
		EXPECT_TRUE(bits.none());
		EXPECT_EQ(bits.count(), 0u);

		bits.flip();
		EXPECT_TRUE(bits.all());
	}

	TEST(Bitset, AnyAllNoneAndCountQueries)
	{
		// any/all/none/count provide summary queries over the entire bitset state.
		std::bitset<6> bits(std::string("001011"));
		EXPECT_TRUE(bits.any());
		EXPECT_FALSE(bits.all());
		EXPECT_FALSE(bits.none());
		EXPECT_EQ(bits.count(), 3u);

		bits.set();
		EXPECT_TRUE(bits.all());
		EXPECT_EQ(bits.count(), 6u);

		bits.reset();
		EXPECT_TRUE(bits.none());
	}

	TEST(Bitset, BitwiseOperatorsAndAssignments)
	{
		// bitset supports &, |, ^ and their assignment forms for mask composition.
		std::bitset<8> a(std::string("11001100"));
		std::bitset<8> b(std::string("10101010"));

		EXPECT_EQ((a & b).to_string(), "10001000");
		EXPECT_EQ((a | b).to_string(), "11101110");
		EXPECT_EQ((a ^ b).to_string(), "01100110");

		std::bitset<8> c = a;
		c &= b;
		EXPECT_EQ(c.to_string(), "10001000");

		c = a;
		c |= b;
		EXPECT_EQ(c.to_string(), "11101110");

		c = a;
		c ^= b;
		EXPECT_EQ(c.to_string(), "01100110");
	}

	TEST(Bitset, ShiftOperatorsAndAssignments)
	{
		// Left/right shifts move bit positions while filling vacated bits with zero.
		std::bitset<8> bits(std::string("00011001"));

		EXPECT_EQ((bits << 2).to_string(), "01100100");
		EXPECT_EQ((bits >> 3).to_string(), "00000011");

		std::bitset<8> moved = bits;
		moved <<= 1;
		EXPECT_EQ(moved.to_string(), "00110010");

		moved >>= 2;
		EXPECT_EQ(moved.to_string(), "00001100");
	}

	TEST(Bitset, ToUnsignedLongAndToUnsignedLongLongConversions)
	{
		// Numeric conversions expose bitset value as unsigned integers. Overflow in
		// target type throws std::overflow_error.
		std::bitset<16> small(std::string("0000000011110000"));
		EXPECT_EQ(small.to_ulong(), 240ul);
		EXPECT_EQ(small.to_ullong(), 240ull);

		std::bitset<128> huge;
		huge.set(100);
		EXPECT_THROW((void)huge.to_ullong(), std::overflow_error);
	}

	TEST(Bitset, ToStringAndStreamInsertionExtraction)
	{
		// to_string and iostream operators provide textual serialization/parsing.
		std::bitset<8> bits(std::string("01011100"));
		EXPECT_EQ(bits.to_string(), "01011100");

		std::ostringstream out;
		out << bits;
		EXPECT_EQ(out.str(), "01011100");

		std::istringstream in("10110001");
		std::bitset<8> parsed;
		in >> parsed;
		EXPECT_EQ(parsed.to_string(), "10110001");
	}

	TEST(Bitset, EqualityAndInequalityComparisons)
	{
		// Equality operators compare full bit patterns and are commonly used in
		// testing/masking scenarios for exact state verification.
		std::bitset<10> first(std::string("1110001110"));
		std::bitset<10> second(std::string("1110001110"));
		std::bitset<10> third(std::string("1110001100"));

		EXPECT_TRUE(first == second);
		EXPECT_FALSE(first != second);
		EXPECT_FALSE(first == third);
		EXPECT_TRUE(first != third);
	}

}  // namespace
