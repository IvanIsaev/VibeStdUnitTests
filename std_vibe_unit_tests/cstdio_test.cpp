#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>

namespace {

TEST(CStdioHeader, SnprintfAndScanfFamily)
{
	// Formatted IO APIs provide portable C-style text serialization/parsing.
	char buffer[64]{};
	const int n = std::snprintf(buffer, sizeof(buffer), "%s %d", "value", 42);
	ASSERT_GT(n, 0);
	EXPECT_STREQ(buffer, "value 42");

	char word[16]{};
	int number = 0;
	EXPECT_EQ(std::sscanf(buffer, "%15s %d", word, &number), 2);
	EXPECT_STREQ(word, "value");
	EXPECT_EQ(number, 42);
}

TEST(CStdioHeader, FileOpenWriteReadAndRemoveRoundtrip)
{
	// Core file APIs: fopen/fwrite/fread/fclose/remove.
	const char* fileName = "cstdio_test_tmp.txt";
	{
		FILE* f = nullptr;
		EXPECT_EQ(fopen_s(&f, fileName, "wb"), 0);
		ASSERT_NE(f, nullptr);
		const char payload[] = "abc123";
		EXPECT_EQ(std::fwrite(payload, 1, std::strlen(payload), f), std::strlen(payload));
		EXPECT_EQ(std::fclose(f), 0);
	}
	{
		FILE* f = nullptr;
		EXPECT_EQ(fopen_s(&f, fileName, "rb"), 0);
		ASSERT_NE(f, nullptr);
		char readBuf[16]{};
		const std::size_t bytes = std::fread(readBuf, 1, sizeof(readBuf), f);
		EXPECT_GT(bytes, 0u);
		EXPECT_EQ(std::fclose(f), 0);
		EXPECT_STREQ(readBuf, "abc123");
	}
	EXPECT_EQ(std::remove(fileName), 0);
}

TEST(CStdioHeader, PositioningAndEOFFunctions)
{
	// fseek/ftell/rewind and EOF checks support random-access file reading.
	const char* fileName = "cstdio_test_pos_tmp.txt";
	FILE* f = nullptr;
	EXPECT_EQ(fopen_s(&f, fileName, "wb+"), 0);
	ASSERT_NE(f, nullptr);

	const char payload[] = "0123456789";
	EXPECT_EQ(std::fwrite(payload, 1, std::strlen(payload), f), std::strlen(payload));
	EXPECT_EQ(std::fseek(f, 5, SEEK_SET), 0);
	EXPECT_EQ(std::ftell(f), 5L);

	const int ch = std::fgetc(f);
	EXPECT_EQ(ch, '5');
	std::rewind(f);
	EXPECT_EQ(std::fgetc(f), '0');

	EXPECT_EQ(std::fclose(f), 0);
	EXPECT_EQ(std::remove(fileName), 0);
}

}  // namespace
