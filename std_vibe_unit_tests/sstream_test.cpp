#include <gtest/gtest.h>

#include <sstream>
#include <string>

namespace {

TEST(SstreamHeader, StringstreamCombinedInputAndOutput)
{
	// stringstream combines formatted insertion and extraction over one buffer.
	std::stringstream ss;
	ss << "alpha " << 42;
	EXPECT_EQ(ss.str(), "alpha 42");

	std::string word;
	int number = 0;
	ss >> word >> number;
	EXPECT_EQ(word, "alpha");
	EXPECT_EQ(number, 42);
}

TEST(SstreamHeader, OstringstreamFormattingAndSeekOperations)
{
	// ostringstream is output-only and supports seek/tell through stringbuf.
	std::ostringstream out;
	out << "hello";
	EXPECT_EQ(out.tellp(), std::streampos(5));
	out.seekp(0);
	out << 'H';
	EXPECT_EQ(out.str(), "Hello");
}

TEST(SstreamHeader, IstringstreamParsingAndStateFlags)
{
	// istringstream provides convenient parsing from in-memory text sources.
	std::istringstream in("10 20 x");
	int a = 0;
	int b = 0;
	char c = '\0';
	in >> a >> b >> c;
	EXPECT_EQ(a, 10);
	EXPECT_EQ(b, 20);
	EXPECT_EQ(c, 'x');
	EXPECT_TRUE(in.good() || in.eof());
}

TEST(SstreamHeader, StringbufDirectBufferManipulation)
{
	// stringbuf offers direct buffer-level API beneath string stream wrappers.
	std::stringbuf buf(std::ios::in | std::ios::out);
	buf.sputn("abc", 3);
	EXPECT_EQ(buf.str(), "abc");
	buf.pubseekpos(1);
	EXPECT_EQ(buf.sgetc(), 'b');
}

}  // namespace
