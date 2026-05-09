#include <gtest/gtest.h>

#include <locale>
#include <sstream>
#include <string>

namespace {

TEST(LocaleHeader, LocaleConstructionAndNameIntrospection)
{
	// locale objects encapsulate cultural formatting and classification facets.
	const std::locale classic = std::locale::classic();
	EXPECT_FALSE(classic.name().empty());

	const std::locale global = std::locale();
	EXPECT_FALSE(global.name().empty());
}

TEST(LocaleHeader, UseFacetAndHasFacetForCtypeAndNumpunct)
{
	// use_facet retrieves strongly typed facet interfaces from locale objects.
	const std::locale loc = std::locale::classic();
	EXPECT_TRUE(std::has_facet<std::ctype<char>>(loc));
	EXPECT_TRUE(std::has_facet<std::numpunct<char>>(loc));

	const auto& ctype = std::use_facet<std::ctype<char>>(loc);
	EXPECT_EQ(ctype.toupper('a'), 'A');
}

TEST(LocaleHeader, StreamImbueAffectsFormattingRules)
{
	// Streams can be imbued with locale to alter punctuation/grouping behavior.
	std::ostringstream oss;
	oss.imbue(std::locale::classic());
	oss << 1234.5;
	EXPECT_FALSE(oss.str().empty());
}

}  // namespace
