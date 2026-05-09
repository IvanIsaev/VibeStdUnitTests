#include <gtest/gtest.h>

#include <istream>
#include <sstream>
#include <string>

namespace {

TEST(IstreamHeader, FormattedExtractionForBuiltins)
{
	// basic_istream formatted extraction parses tokenized textual input.
	std::istringstream in("123 4.5 token");
	int i = 0;
	double d = 0.0;
	std::string s;
	in >> i >> d >> s;
	EXPECT_EQ(i, 123);
	EXPECT_DOUBLE_EQ(d, 4.5);
	EXPECT_EQ(s, "token");
}

TEST(IstreamHeader, UnformattedInputGetReadAndGcount)
{
	// Unformatted functions read raw characters and track extraction counts.
	std::istringstream in("abcdef");
	char buf[4]{};
	in.read(buf, 3);
	EXPECT_EQ(in.gcount(), 3);
	EXPECT_EQ(std::string(buf, 3), "abc");

	char ch = '\0';
	in.get(ch);
	EXPECT_EQ(ch, 'd');
}

TEST(IstreamHeader, IgnorePeekPutbackAndUngetBehavior)
{
	// Peek and putback/unget allow limited cursor control in input streams.
	std::istringstream in("xyz");
	EXPECT_EQ(in.peek(), 'x');
	char ch = '\0';
	in.get(ch);
	EXPECT_EQ(ch, 'x');
	in.putback(ch);
	EXPECT_EQ(in.get(), 'x');
	in.unget();
	EXPECT_EQ(in.get(), 'x');

	in.ignore(2);
	EXPECT_TRUE(in.eof());
}

}  // namespace
