#include <gtest/gtest.h>

#include <ostream>
#include <sstream>

namespace {

TEST(OstreamHeader, FormattedInsertionAndManipulators)
{
	// basic_ostream formatted insertion covers primitive and string output.
	std::ostringstream out;
	out << "value=" << 42 << ' ' << std::hex << 255;
	EXPECT_EQ(out.str(), "value=42 ff");
}

TEST(OstreamHeader, UnformattedOutputPutWriteFlush)
{
	// put/write emit raw characters while flush synchronizes buffered output.
	std::ostringstream out;
	out.put('A');
	out.write("BC", 2);
	out.flush();
	EXPECT_EQ(out.str(), "ABC");
}

TEST(OstreamHeader, SeekAndTellWithStringStreamBuffer)
{
	// When tied to seekable stream buffers, ostream can reposition write cursor.
	std::ostringstream out;
	out << "hello";
	EXPECT_EQ(out.tellp(), std::streampos(5));
	out.seekp(0);
	out << 'H';
	EXPECT_EQ(out.str(), "Hello");
}

}  // namespace
