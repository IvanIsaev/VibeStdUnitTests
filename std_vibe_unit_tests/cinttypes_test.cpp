#include <gtest/gtest.h>

#include <cinttypes>
#include <cstdio>
#include <cstdint>

namespace {

TEST(CInttypesHeader, IntegerFormatMacrosWithSnprintf)
{
	// PRI* macros provide portable printf format specifiers for fixed-width ints.
	char buffer[64]{};
	const std::int64_t value = 1234567890123LL;
	const int n = std::snprintf(buffer, sizeof(buffer), "%" PRId64, value);
	ASSERT_GT(n, 0);
	EXPECT_STREQ(buffer, "1234567890123");
}

TEST(CInttypesHeader, IntegerScanMacrosWithSscanf)
{
	// SCN* macros provide portable scanf specifiers for fixed-width ints.
	const char* text = "987654321";
	std::int32_t parsed = 0;
	const int n = std::sscanf(text, "%" SCNd32, &parsed);
	EXPECT_EQ(n, 1);
	EXPECT_EQ(parsed, 987654321);
}

TEST(CInttypesHeader, ImaxabsImaxdivAndStrtoimax)
{
	// <cinttypes> also includes intmax_t-oriented arithmetic helpers.
	EXPECT_EQ(std::imaxabs(static_cast<std::intmax_t>(-77)), 77);

	const auto qr = std::imaxdiv(static_cast<std::intmax_t>(17), static_cast<std::intmax_t>(5));
	EXPECT_EQ(qr.quot, 3);
	EXPECT_EQ(qr.rem, 2);

	char* end = nullptr;
	const std::intmax_t parsed = std::strtoimax("1234", &end, 10);
	EXPECT_EQ(parsed, 1234);
	EXPECT_EQ(*end, '\0');
}

}  // namespace
