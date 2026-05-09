#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace {

TEST(StringViewHeader, ConstructionExtentAndElementAccess)
{
	// string_view is a non-owning view that never allocates or mutates storage.
	std::string source = "hello world";
	std::string_view sv(source);
	EXPECT_EQ(sv.size(), 11u);
	EXPECT_EQ(sv.front(), 'h');
	EXPECT_EQ(sv.back(), 'd');
	EXPECT_EQ(sv[1], 'e');
	EXPECT_EQ(sv.at(4), 'o');
	EXPECT_THROW((void)sv.at(20), std::out_of_range);
}

TEST(StringViewHeader, PrefixSuffixAndSubstrOperations)
{
	// remove_prefix/remove_suffix adjust the viewed window without copying.
	std::string_view sv = "alpha.beta.gamma";
	EXPECT_TRUE(sv.starts_with("alpha"));
	EXPECT_TRUE(sv.ends_with("gamma"));
	EXPECT_EQ(sv.substr(6, 4), "beta");

	sv.remove_prefix(6);
	EXPECT_EQ(sv, "beta.gamma");
	sv.remove_suffix(6);
	EXPECT_EQ(sv, "beta");
}

TEST(StringViewHeader, FindCompareAndContainsSemantics)
{
	// compare/find/rfind/find_first_of family provide lexical search behavior.
	std::string_view sv = "one two three two";
	EXPECT_EQ(sv.find("two"), 4u);
	EXPECT_EQ(sv.rfind("two"), 14u);
	EXPECT_EQ(sv.find_first_of("xyz"), std::string_view::npos);
	EXPECT_EQ(sv.compare("one two three two"), 0);
	EXPECT_TRUE(sv.contains("three"));
}

TEST(StringViewHeader, LiteralsAndInteroperabilityWithString)
{
	// "sv" literal creates compile-time string_view constants.
	using namespace std::literals;
	constexpr std::string_view lit = "text"sv;
	static_assert(lit.size() == 4);
	EXPECT_EQ(lit, "text");

	std::string dynamic = "dynamic";
	std::string_view view(dynamic);
	EXPECT_EQ(view, "dynamic");
}

}  // namespace
