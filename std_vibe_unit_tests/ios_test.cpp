#include <gtest/gtest.h>

#include <ios>
#include <sstream>

namespace {

TEST(IosHeader, FormatFlagsAndOpenmodeBitmasks)
{
	// ios_base defines bitmask enums controlling formatting and open behavior.
	const auto mode = std::ios::in | std::ios::out | std::ios::binary;
	EXPECT_TRUE((mode & std::ios::in) != 0);
	EXPECT_TRUE((mode & std::ios::binary) != 0);

	std::ostringstream oss;
	oss.setf(std::ios::hex, std::ios::basefield);
	oss << 26;
	EXPECT_EQ(oss.str(), "1a");
}

TEST(IosHeader, StreamStateFlagsGoodFailEofBad)
{
	// basic_ios tracks stream health through good/fail/eof/bad states.
	std::istringstream in("1");
	int value = 0;
	in >> value;
	EXPECT_TRUE(in.good() || in.eof());
	EXPECT_EQ(value, 1);

	// Trigger failbit by attempting to parse non-numeric input as integer.
	std::istringstream badIn("x");
	badIn >> value;
	EXPECT_TRUE(badIn.fail());
}

TEST(IosHeader, ExceptionsAndIostateMask)
{
	// Streams can be configured to throw exceptions when certain state bits set.
	std::istringstream in("abc");
	in.exceptions(std::ios::failbit);
	int value = 0;
	EXPECT_THROW(in >> value, std::ios_base::failure);
}

}  // namespace
