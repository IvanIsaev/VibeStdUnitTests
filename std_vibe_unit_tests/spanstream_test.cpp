#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<spanstream>)
#include <spanstream>
#define VIBE_HAS_SPANSTREAM 1
#else
#define VIBE_HAS_SPANSTREAM 0
#endif

#include <array>
#include <span>
#include <string>

namespace {

TEST(SpanstreamHeader, HeaderAvailabilityAndFeatureMacro)
{
	// <spanstream> is C++23 and may be absent on some standard libraries.
#if VIBE_HAS_SPANSTREAM
#ifdef __cpp_lib_spanstream
	EXPECT_GE(__cpp_lib_spanstream, 202106L);
#endif
#else
	GTEST_SKIP() << "<spanstream> is not available in this standard library.";
#endif
}

#if VIBE_HAS_SPANSTREAM

TEST(SpanstreamHeader, IspanstreamReadsFromProvidedSpan)
{
	// ispanstream reads from caller-owned contiguous character storage.
	std::array<char, 16> storage{ '1', '2', ' ', '3', '4', '\0' };
	std::ispanstream in(std::span<const char>(storage.data(), 5));
	int a = 0;
	int b = 0;
	in >> a >> b;
	EXPECT_EQ(a, 12);
	EXPECT_EQ(b, 34);
}

TEST(SpanstreamHeader, OspanstreamWritesIntoProvidedSpan)
{
	// ospanstream writes without allocation into a fixed external span.
	std::array<char, 32> storage{};
	std::ospanstream out(std::span<char>(storage));
	out << "abc" << 123;
	out.flush();
	EXPECT_TRUE(std::string(storage.data()).starts_with("abc123"));
}

TEST(SpanstreamHeader, SpanstreamSupportsCombinedReadWrite)
{
	// spanstream combines input and output over a single external buffer.
	std::array<char, 32> storage{};
	std::spanstream io(std::span<char>(storage));
	io << "7 8";
	io.seekg(0);
	int a = 0;
	int b = 0;
	io >> a >> b;
	EXPECT_EQ(a, 7);
	EXPECT_EQ(b, 8);
}

#endif

}  // namespace
