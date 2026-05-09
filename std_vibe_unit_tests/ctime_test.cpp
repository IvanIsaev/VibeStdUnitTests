#include <gtest/gtest.h>

#include <ctime>

namespace {

TEST(CTimeHeader, TimeNowAndDifferenceFunctions)
{
	// C time APIs provide epoch-based wall-clock snapshots and differences.
	const std::time_t t1 = std::time(nullptr);
	ASSERT_NE(t1, static_cast<std::time_t>(-1));
	const std::time_t t2 = std::time(nullptr);
	ASSERT_NE(t2, static_cast<std::time_t>(-1));
	EXPECT_GE(std::difftime(t2, t1), 0.0);
}

TEST(CTimeHeader, BrokenDownTimeConversion)
{
	// localtime/gmtime convert epoch time to calendar components.
	const std::time_t now = std::time(nullptr);
	ASSERT_NE(now, static_cast<std::time_t>(-1));

	std::tm localTm{};
	EXPECT_EQ(localtime_s(&localTm, &now), 0);
	EXPECT_GE(localTm.tm_mon, 0);
	EXPECT_LE(localTm.tm_mon, 11);
	EXPECT_GE(localTm.tm_mday, 1);
	EXPECT_LE(localTm.tm_mday, 31);
}

TEST(CTimeHeader, FormattingWithStrftime)
{
	// strftime formats broken-down time into textual representation.
	const std::time_t now = std::time(nullptr);
	ASSERT_NE(now, static_cast<std::time_t>(-1));

	std::tm localTm{};
	EXPECT_EQ(localtime_s(&localTm, &now), 0);

	char buffer[64]{};
	const std::size_t written = std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &localTm);
	EXPECT_GT(written, 0u);
}

}  // namespace
