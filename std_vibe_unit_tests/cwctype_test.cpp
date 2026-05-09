#include <gtest/gtest.h>

#include <cwctype>

namespace {

TEST(CWctypeHeader, ClassificationFunctions)
{
	// Wide-character classification functions operate on wint_t values.
	EXPECT_TRUE(std::iswalpha(L'A'));
	EXPECT_TRUE(std::iswdigit(L'9'));
	EXPECT_TRUE(std::iswspace(L' '));
	EXPECT_TRUE(std::iswpunct(L'!'));
	EXPECT_FALSE(std::iswlower(L'Q'));
}

TEST(CWctypeHeader, CaseMappingAndCharacterTypeLookup)
{
	// towupper/towlower and wctype/iswctype provide dynamic classification.
	EXPECT_EQ(std::towupper(L'a'), L'A');
	EXPECT_EQ(std::towlower(L'Z'), L'z');

	const std::wctype_t alpha = std::wctype("alpha");
	ASSERT_NE(alpha, 0);
	EXPECT_TRUE(std::iswctype(L'M', alpha));
}

}  // namespace
