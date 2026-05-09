#include <gtest/gtest.h>

#include <array>
#include <charconv>
#include <system_error>

namespace {

TEST(CharconvHeader, ToCharsForIntegralTypes)
{
	// to_chars writes numeric text into caller-provided buffer without allocation.
	std::array<char, 32> buffer{};
	auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), 12345);
	EXPECT_EQ(ec, std::errc{});
	ASSERT_NE(ptr, buffer.data());
	*ptr = '\0';
	EXPECT_STREQ(buffer.data(), "12345");
}

TEST(CharconvHeader, FromCharsForIntegralTypes)
{
	// from_chars parses numeric text with predictable, locale-independent behavior.
	const char* text = "6789";
	int value = 0;
	auto [ptr, ec] = std::from_chars(text, text + 4, value);
	EXPECT_EQ(ec, std::errc{});
	EXPECT_EQ(ptr, text + 4);
	EXPECT_EQ(value, 6789);
}

TEST(CharconvHeader, ErrorHandlingOnInvalidInput)
{
	// Parsing failure reports std::errc::invalid_argument and leaves value intact.
	const char* bad = "abc";
	int value = 42;
	auto [ptr, ec] = std::from_chars(bad, bad + 3, value);
	EXPECT_EQ(ec, std::errc::invalid_argument);
	EXPECT_EQ(ptr, bad);
	EXPECT_EQ(value, 42);
}

TEST(CharconvHeader, FloatingPointConversionsWhenAvailable)
{
	// Floating-point charconv support is available on modern implementations.
#ifdef __cpp_lib_to_chars
	std::array<char, 64> buffer{};
	auto [out, ec1] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), 3.5);
	if (ec1 != std::errc{})
	{
		GTEST_SKIP() << "Floating to_chars not implemented on this standard library.";
	}
	double value = 0.0;
	auto [in, ec2] = std::from_chars(buffer.data(), out, value);
	EXPECT_EQ(ec2, std::errc{});
	EXPECT_EQ(in, out);
	EXPECT_NEAR(value, 3.5, 1e-9);
#endif
}

}  // namespace
