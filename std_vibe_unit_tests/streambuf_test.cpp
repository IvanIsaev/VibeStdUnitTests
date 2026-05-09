#include <gtest/gtest.h>

#include <sstream>
#include <streambuf>
#include <string>

namespace {

// basic_streambuf is abstract; stringbuf is the usual concrete char stream buffer.
TEST(StreambufHeader, StringbufPutAreaAndStringAccess)
{
	// basic_streambuf maintains put/get areas; stringbuf maps them to std::string.
	std::stringbuf buf;
	buf.sputn("hello", 5);
	EXPECT_EQ(buf.str(), "hello");
}

TEST(StreambufHeader, PubSeekOffAndPubSeekPosRepositionGetPointer)
{
	// pubseekoff/pubseekpos expose seekable stream buffer positioning.
	std::stringbuf buf(std::ios::in | std::ios::out);
	buf.str("abcdef");
	EXPECT_EQ(buf.pubseekoff(3, std::ios::beg, std::ios::in), std::streampos(3));
	EXPECT_EQ(buf.sgetc(), 'd');
}

TEST(StreambufHeader, GetAreaSequentialReads)
{
	// sgetc/snextc/sbumpc read from the get area; sgetn reads a run of chars.
	std::stringbuf buf;
	buf.str("xyz");
	EXPECT_EQ(buf.sgetc(), 'x');
	EXPECT_EQ(buf.sbumpc(), 'x');
	EXPECT_EQ(buf.sgetc(), 'y');

	char out[2]{};
	EXPECT_EQ(buf.sgetn(out, 2), 2);
	EXPECT_EQ(std::string(out, 2), "yz");
}

TEST(StreambufHeader, PutbackAndSungetc)
{
	// sputbackc/sungetc move the get pointer backward when allowed.
	std::stringbuf buf;
	buf.str("ab");
	EXPECT_EQ(buf.sbumpc(), 'a');
	EXPECT_EQ(buf.sputbackc('a'), 'a');
	EXPECT_EQ(buf.sgetc(), 'a');
}

TEST(StreambufHeader, PubsyncFlushesAssociatedOutput)
{
	// pubsync (called by flush) lets derived buffers sync external devices.
	std::stringbuf buf;
	buf.sputc('z');
	EXPECT_EQ(buf.pubsync(), 0);
}

}  // namespace
