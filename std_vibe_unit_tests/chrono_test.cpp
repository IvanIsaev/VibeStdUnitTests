#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace {

TEST(ChronoHeader, DurationArithmeticAndCasts)
{
	// duration types model strongly typed time intervals with ratio periods.
	using namespace std::chrono;
	const seconds s(2);
	const milliseconds ms = duration_cast<milliseconds>(s);
	EXPECT_EQ(ms.count(), 2000);

	const auto mixed = milliseconds(1500) + seconds(1);
	EXPECT_EQ(mixed.count(), 2500);
}

TEST(ChronoHeader, TimePointAndClockUsage)
{
	// time_point binds a duration to a specific clock epoch.
	using namespace std::chrono;
	const auto t1 = steady_clock::now();
	std::this_thread::sleep_for(milliseconds(1));
	const auto t2 = steady_clock::now();
	EXPECT_GE(duration_cast<microseconds>(t2 - t1).count(), 0);
}

TEST(ChronoHeader, CalendarAndTimezoneIndependentUtilities)
{
	// C++20 calendar types support civil date decomposition/composition.
	using namespace std::chrono;
	const year_month_day ymd = 2026y / May / 9d;
	EXPECT_TRUE(ymd.ok());
	EXPECT_EQ(static_cast<int>(ymd.year()), 2026);
	EXPECT_EQ(static_cast<unsigned>(ymd.month()), static_cast<unsigned>(May));
	EXPECT_EQ(static_cast<unsigned>(ymd.day()), 9u);

	const sys_days days(ymd);
	const year_month_day roundtrip(days);
	EXPECT_EQ(roundtrip, ymd);
}

}  // namespace
