#include <gtest/gtest.h>

#include <cerrno>
#include <cstring>

namespace {

TEST(CStringHeader, LengthCopyAndConcatenationFunctions)
{
	// Traditional C string APIs operate on null-terminated char buffers.
	const char* text = "hello";
	EXPECT_EQ(std::strlen(text), 5u);

	char buffer[16]{};
	std::strcpy(buffer, "abc");
	EXPECT_STREQ(buffer, "abc");

	std::strcat(buffer, "def");
	EXPECT_STREQ(buffer, "abcdef");
}

TEST(CStringHeader, CompareAndSpanSearchFunctions)
{
	// strcmp/strncmp and character-search APIs are central text primitives.
	EXPECT_LT(std::strcmp("abc", "abd"), 0);
	EXPECT_EQ(std::strncmp("abcdef", "abcxyz", 3), 0);

	const char* s = "sample";
	EXPECT_EQ(*std::strchr(s, 'm'), 'm');
	EXPECT_EQ(*std::strrchr(s, 'a'), 'a');
	EXPECT_EQ(std::strspn("abc123", "abc"), 3u);
	EXPECT_EQ(std::strcspn("abc123", "123"), 3u);
}

TEST(CStringHeader, MemoryBlockManipulationFunctions)
{
	// memcpy/memmove/memset/memcmp provide raw byte block manipulation.
	char src[8] = { 'a', 'b', 'c', '\0', 'x', 'y', 'z', '\0' };
	char dst[8]{};
	std::memcpy(dst, src, sizeof(src));
	EXPECT_EQ(std::memcmp(dst, src, sizeof(src)), 0);

	std::memset(dst, 0, sizeof(dst));
	EXPECT_EQ(dst[0], '\0');

	char overlap[8] = { '1', '2', '3', '4', '5', '\0', '\0', '\0' };
	std::memmove(overlap + 1, overlap, 4);
	EXPECT_EQ(overlap[1], '1');
	EXPECT_EQ(overlap[4], '4');
}

TEST(CStringHeader, TokenizationAndErrorMessageFunctions)
{
	// strtok splits a mutable buffer into delimiter-separated tokens.
	char text[] = "a,b,c";
	char* tok1 = std::strtok(text, ",");
	ASSERT_NE(tok1, nullptr);
	EXPECT_STREQ(tok1, "a");
	char* tok2 = std::strtok(nullptr, ",");
	ASSERT_NE(tok2, nullptr);
	EXPECT_STREQ(tok2, "b");

	const char* err = std::strerror(EDOM);
	ASSERT_NE(err, nullptr);
	EXPECT_GT(std::strlen(err), 0u);
}

}  // namespace
