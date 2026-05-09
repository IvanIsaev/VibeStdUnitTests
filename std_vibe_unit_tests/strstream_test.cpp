#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<strstream>)
#include <strstream>
#define VIBE_HAS_STRSTREAM 1
#else
#define VIBE_HAS_STRSTREAM 0
#endif

#include <cstring>

namespace {

TEST(StrstreamHeader, HeaderAvailability)
{
	// <strstream> is deprecated and may be removed from newer standard libraries.
#if VIBE_HAS_STRSTREAM
	SUCCEED();
#else
	GTEST_SKIP() << "<strstream> is not available in this standard library.";
#endif
}

#if VIBE_HAS_STRSTREAM

TEST(StrstreamHeader, IstrstreamFormattedExtraction)
{
	// istrstream reads from a caller-owned char buffer without copying ownership.
	char data[] = "42 pi";
	std::istrstream in(data);
	int n = 0;
	char word[8]{};
	in >> n >> word;
	EXPECT_EQ(n, 42);
	EXPECT_STREQ(word, "pi");
}

TEST(StrstreamHeader, OstrstreamWriteAndStrFreeze)
{
	// ostrstream accumulates into a growable buffer; freeze controls ownership.
	std::ostrstream out;
	out << "n=" << 7;
	out << std::ends;
	char* p = out.str();
	ASSERT_NE(p, nullptr);
	EXPECT_NE(std::strstr(p, "n=7"), nullptr);
	out.freeze(false);
}

TEST(StrstreamHeader, StrstreamBidirectionalOnBuffer)
{
	// strstream supports both insertion and extraction on one buffer.
	char storage[32]{};
	std::strstream ss(storage, sizeof(storage), std::ios::in | std::ios::out);
	ss << 100 << ' ' << "ok";
	ss.seekg(0);
	int v = 0;
	char w[8]{};
	ss >> v >> w;
	EXPECT_EQ(v, 100);
	EXPECT_STREQ(w, "ok");
}

#endif

}  // namespace
