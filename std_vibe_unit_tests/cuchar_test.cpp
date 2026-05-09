#include <gtest/gtest.h>

#include <cuchar>
#include <cstring>

namespace {

TEST(CUcharHeader, Mbrtoc16AndC16rtombRoundtrip)
{
	// <cuchar> provides conversion between multibyte and UTF-16/UTF-32 units.
	const char* mb = "A";
	std::mbstate_t state{};
	char16_t out = 0;
	const std::size_t consumed = std::mbrtoc16(&out, mb, std::strlen(mb), &state);
	ASSERT_NE(consumed, static_cast<std::size_t>(-1));
	ASSERT_NE(consumed, static_cast<std::size_t>(-2));
	EXPECT_EQ(out, u'A');

	char buffer[8]{};
	std::mbstate_t state2{};
	const std::size_t produced = std::c16rtomb(buffer, u'A', &state2);
	ASSERT_NE(produced, static_cast<std::size_t>(-1));
	EXPECT_GT(produced, 0u);
	EXPECT_EQ(buffer[0], 'A');
}

}  // namespace
