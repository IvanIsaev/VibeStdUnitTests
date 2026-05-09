#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace {

TEST(SpanHeader, ConstructionFromArrayVectorAndPointerCount)
{
	// std::span is a non-owning view over contiguous memory and can bind to many
	// contiguous sources without copying.
	std::array<int, 4> arr{ 1, 2, 3, 4 };
	std::span<int> fromArray(arr);
	EXPECT_EQ(fromArray.size(), 4u);
	EXPECT_EQ(fromArray[0], 1);

	std::vector<int> vec{ 5, 6, 7 };
	std::span<int> fromVector(vec);
	EXPECT_EQ(fromVector.size(), 3u);
	EXPECT_EQ(fromVector.back(), 7);

	std::span<int> fromPtr(arr.data(), 2);
	EXPECT_EQ(fromPtr.size(), 2u);
	EXPECT_EQ(fromPtr[1], 2);
}

TEST(SpanHeader, StaticExtentAndCompileTimeProperties)
{
	// span<T, N> encodes extent in the type; span<T> carries dynamic extent.
	using StaticSpan = std::span<int, 3>;
	EXPECT_EQ(StaticSpan::extent, 3u);
	EXPECT_EQ(std::span<int>::extent, std::dynamic_extent);

	int values[3] = { 10, 20, 30 };
	StaticSpan s(values);
	EXPECT_EQ(s.size(), 3u);
	EXPECT_EQ(s[2], 30);
}

TEST(SpanHeader, ElementAccessAndIterators)
{
	// front/back/operator[]/at()/data() and iterator family provide safe and
	// convenient access semantics.
	std::array<int, 5> values{ 1, 2, 3, 4, 5 };
	std::span<int> s(values);

	EXPECT_EQ(s.front(), 1);
	EXPECT_EQ(s.back(), 5);
	EXPECT_EQ(s[2], 3);
#if defined(__cpp_lib_span) && (__cpp_lib_span >= 202311L)
	EXPECT_EQ(s.at(4), 5);
	EXPECT_THROW((void)s.at(5), std::out_of_range);
#endif
	EXPECT_EQ(*s.data(), 1);

	int sum = 0;
	for (auto it = s.begin(); it != s.end(); ++it)
	{
		sum += *it;
	}
	EXPECT_EQ(sum, 15);
}

TEST(SpanHeader, SubviewsFirstLastAndSubspan)
{
	// first/last/subspan provide cheap slicing operations over the same buffer.
	int values[] = { 0, 1, 2, 3, 4, 5 };
	std::span<int> s(values);

	auto first3 = s.first<3>();
	EXPECT_EQ((std::vector<int>(first3.begin(), first3.end())), (std::vector<int>{ 0, 1, 2 }));

	auto last2 = s.last(2);
	EXPECT_EQ((std::vector<int>(last2.begin(), last2.end())), (std::vector<int>{ 4, 5 }));

	auto middle = s.subspan(2, 3);
	EXPECT_EQ((std::vector<int>(middle.begin(), middle.end())), (std::vector<int>{ 2, 3, 4 }));
}

TEST(SpanHeader, ByteViewsWhenAvailable)
{
	// as_bytes/as_writable_bytes expose raw object representation over spans.
	std::array<std::uint16_t, 2> values{ 0x1122u, 0x3344u };
	std::span<std::uint16_t> s(values);

	auto bytes = std::as_bytes(s);
	EXPECT_EQ(bytes.size(), sizeof(std::uint16_t) * values.size());

	auto writable = std::as_writable_bytes(s);
	EXPECT_EQ(writable.size(), bytes.size());
}

}  // namespace
