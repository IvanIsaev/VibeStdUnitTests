#include <gtest/gtest.h>

#include <cctype>

namespace {

TEST(CCtypeHeader, ClassificationFunctions)
{
	// cctype classification predicates inspect character categories.
	EXPECT_TRUE(std::isalpha('A'));
	EXPECT_TRUE(std::isdigit('9'));
	EXPECT_TRUE(std::isalnum('z'));
	EXPECT_TRUE(std::isspace(' '));
	EXPECT_TRUE(std::ispunct('!'));
	EXPECT_FALSE(std::isupper('a'));
}

TEST(CCtypeHeader, CaseConversionFunctions)
{
	// tolower/toupper perform locale-independent ASCII-oriented case mapping.
	EXPECT_EQ(std::toupper('a'), 'A');
	EXPECT_EQ(std::tolower('Z'), 'z');
	EXPECT_EQ(std::toupper('1'), '1');
}

}  // namespace
