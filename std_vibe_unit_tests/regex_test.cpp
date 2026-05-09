#include <gtest/gtest.h>

#include <regex>
#include <string>
#include <vector>

namespace {

TEST(RegexHeader, RegexMatchAndRegexSearchBasics)
{
	// regex_match checks full-string matches, while regex_search finds substrings.
	const std::regex whole(R"(\d{4}-\d{2}-\d{2})");
	EXPECT_TRUE(std::regex_match("2026-05-09", whole));
	EXPECT_FALSE(std::regex_match("date:2026-05-09", whole));

	std::smatch match;
	EXPECT_TRUE(std::regex_search("id=42;", match, std::regex(R"(id=(\d+))")));
	EXPECT_EQ(match[1].str(), "42");
}

TEST(RegexHeader, IterationOverMultipleMatches)
{
	// sregex_iterator traverses all non-overlapping matches in a string.
	const std::string text = "a1 b22 c333";
	const std::regex digits(R"(\d+)");
	std::vector<std::string> tokens;
	for (std::sregex_iterator it(text.begin(), text.end(), digits), end; it != end; ++it)
	{
		tokens.push_back(it->str());
	}
	EXPECT_EQ(tokens, (std::vector<std::string>{ "1", "22", "333" }));
}

TEST(RegexHeader, RegexReplaceAndFlags)
{
	// regex_replace transforms matches and supports formatting flags.
	const std::string text = "x=1;y=2";
	const std::regex keyValue(R"((\w)=(\d))");
	const std::string replaced = std::regex_replace(text, keyValue, "$1:$2");
	EXPECT_EQ(replaced, "x:1;y:2");

	const std::string firstOnly = std::regex_replace(text, keyValue, "pair", std::regex_constants::format_first_only);
	EXPECT_EQ(firstOnly, "pair;y=2");
}

TEST(RegexHeader, ErrorHandlingForInvalidPatterns)
{
	// Invalid regex patterns throw std::regex_error with classification code.
	try
	{
		const std::regex bad("(");
		(void)bad;
		FAIL() << "Expected std::regex_error.";
	}
	catch (const std::regex_error& ex)
	{
		EXPECT_NE(ex.code(), std::regex_constants::error_type{});
	}
}

}  // namespace
