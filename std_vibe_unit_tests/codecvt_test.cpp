#include <gtest/gtest.h>

#include <codecvt>
#include <locale>
#include <string>

namespace {

TEST(CodecvtHeader, Utf8Utf16ConversionFacetBasics)
{
	// codecvt facets are deprecated in C++17+ but still part of the standard.
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
	const std::u16string utf16 = u"Hello";
	const std::string utf8 = conv.to_bytes(utf16);
	EXPECT_EQ(utf8, "Hello");

	const std::u16string roundtrip = conv.from_bytes(utf8);
	EXPECT_EQ(roundtrip, utf16);
}

}  // namespace
