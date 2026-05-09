#include <gtest/gtest.h>

#include <iostream>
#include <sstream>

namespace {

TEST(IostreamHeader, StandardStreamObjectsAreUsable)
{
	// <iostream> defines the global standard stream objects and ties.
	EXPECT_NE(std::cin.tie(), nullptr);
	EXPECT_TRUE(std::cout.good());
	EXPECT_TRUE(std::cerr.good());
	EXPECT_TRUE(std::clog.good());
}

TEST(IostreamHeader, SyncWithStdioControlReturnsPreviousState)
{
	// sync_with_stdio toggles synchronization with C stdio and returns old state.
	const bool oldState = std::ios::sync_with_stdio(false);
	const bool restoreReturn = std::ios::sync_with_stdio(oldState);
	EXPECT_EQ(restoreReturn, false);
}

TEST(IostreamHeader, StreamInsertionAndExtractionViaStringBuffer)
{
	// iostream operators are typically tested through string-backed buffers.
	std::stringstream ss;
	ss << "value " << 42;
	EXPECT_EQ(ss.str(), "value 42");

	std::string word;
	int number = 0;
	ss >> word >> number;
	EXPECT_EQ(word, "value");
	EXPECT_EQ(number, 42);
}

}  // namespace
