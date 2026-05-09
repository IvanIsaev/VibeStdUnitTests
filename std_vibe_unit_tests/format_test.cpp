#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<format>)
#include <format>
#define VIBE_HAS_FORMAT 1
#else
#define VIBE_HAS_FORMAT 0
#endif

#include <string>

namespace {

TEST(FormatHeader, HeaderAvailabilityAndFeatureMacro)
{
	// <format> can be missing on some standard-library/compiler combinations.
#if VIBE_HAS_FORMAT
#ifdef __cpp_lib_format
	EXPECT_GE(__cpp_lib_format, 201907L);
#endif
#else
	GTEST_SKIP() << "<format> is not available in this standard library.";
#endif
}

#if VIBE_HAS_FORMAT

TEST(FormatHeader, BasicFormattingAndPositionalArguments)
{
	// std::format provides Python-like formatting with strong type safety.
	const std::string text = std::format("Hello {} {}", "C++", 23);
	EXPECT_EQ(text, "Hello C++ 23");

	const std::string positional = std::format("{1} then {0}", "first", "second");
	EXPECT_EQ(positional, "second then first");
}

TEST(FormatHeader, FormatToAndFormatToNOutputPaths)
{
	// format_to writes into output iterators; format_to_n limits output length.
	std::string out;
	std::format_to(std::back_inserter(out), "{}-{}", 10, 20);
	EXPECT_EQ(out, "10-20");

	char buffer[5]{};
	auto result = std::format_to_n(buffer, 4, "{}", 123456);
	EXPECT_GE(result.size, 6u);
	EXPECT_STREQ(buffer, "1234");
}

TEST(FormatHeader, FormattedSizeAndRuntimeFormatString)
{
	// formatted_size predicts produced text length and runtime allows dynamic specs.
	const auto size = std::formatted_size("{} + {} = {}", 2, 3, 5);
	EXPECT_EQ(size, 9u);

	const std::string pattern = "{}:{}";
	const std::string text = std::vformat(pattern, std::make_format_args("x", 7));
	EXPECT_EQ(text, "x:7");
}

#endif

}  // namespace
