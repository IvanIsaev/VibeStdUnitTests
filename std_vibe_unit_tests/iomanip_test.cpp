#include <gtest/gtest.h>

#include <iomanip>
#include <sstream>

namespace {

TEST(IomanipHeader, WidthFillAndAlignmentManipulators)
{
	// iomanip exposes formatting manipulators that alter stream presentation.
	std::ostringstream oss;
	oss << std::setfill('0') << std::setw(5) << 42;
	EXPECT_EQ(oss.str(), "00042");

	std::ostringstream left;
	left << std::left << std::setw(6) << "ab";
	EXPECT_EQ(left.str(), "ab    ");
}

TEST(IomanipHeader, PrecisionFixedAndScientificFormatting)
{
	// fixed/scientific and precision control floating-point text rendering.
	std::ostringstream fixed;
	fixed << std::fixed << std::setprecision(2) << 3.14159;
	EXPECT_EQ(fixed.str(), "3.14");

	std::ostringstream sci;
	sci << std::scientific << std::setprecision(1) << 12.0;
	EXPECT_TRUE(sci.str().find('e') != std::string::npos || sci.str().find('E') != std::string::npos);
}

TEST(IomanipHeader, BaseAndBooleanManipulators)
{
	// Basefield and boolalpha manipulators alter integer and bool formatting.
	std::ostringstream hex;
	hex << std::hex << 255;
	EXPECT_EQ(hex.str(), "ff");

	std::ostringstream bools;
	bools << std::boolalpha << true << ' ' << false;
	EXPECT_EQ(bools.str(), "true false");
}

}  // namespace
