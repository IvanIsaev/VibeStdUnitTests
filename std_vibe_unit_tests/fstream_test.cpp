#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace {

TEST(FstreamHeader, OfstreamWritesAndIfstreamReadsText)
{
	// <fstream> provides file-backed stream types for persistent IO.
	const auto file = std::filesystem::temp_directory_path() / "vibe_fstream_text.txt";
	{
		std::ofstream out(file, std::ios::out | std::ios::trunc);
		ASSERT_TRUE(out.is_open());
		out << "line1\nline2";
	}
	{
		std::ifstream in(file);
		ASSERT_TRUE(in.is_open());
		std::string line1;
		std::string line2;
		std::getline(in, line1);
		std::getline(in, line2);
		EXPECT_EQ(line1, "line1");
		EXPECT_EQ(line2, "line2");
	}
	std::error_code ec;
	std::filesystem::remove(file, ec);
}

TEST(FstreamHeader, BinaryModeReadWriteAndSeekOperations)
{
	// fstream supports random access and binary read/write workflows.
	const auto file = std::filesystem::temp_directory_path() / "vibe_fstream_bin.bin";
	{
		std::fstream fs(file, std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
		ASSERT_TRUE(fs.is_open());
		const char payload[] = { 'A', 'B', 'C', 'D' };
		fs.write(payload, 4);
		fs.seekg(1, std::ios::beg);
		char ch = '\0';
		fs.read(&ch, 1);
		EXPECT_EQ(ch, 'B');
		fs.seekp(2, std::ios::beg);
		fs.put('Z');
	}
	{
		std::ifstream in(file, std::ios::binary);
		ASSERT_TRUE(in.good());
		std::string text(4, '\0');
		in.read(text.data(), 4);
		EXPECT_EQ(text, "ABZD");
	}
	std::error_code ec;
	std::filesystem::remove(file, ec);
}

TEST(FstreamHeader, OpenModesAndStateFlags)
{
	// ios_base openmode flags drive append/trunc/ate semantics for streams.
	const auto file = std::filesystem::temp_directory_path() / "vibe_fstream_modes.txt";
	{
		std::ofstream out(file, std::ios::out | std::ios::trunc);
		ASSERT_TRUE(out.good());
		out << "abc";
	}
	{
		std::ofstream out(file, std::ios::out | std::ios::app);
		ASSERT_TRUE(out.good());
		out << "def";
	}
	{
		std::ifstream in(file);
		ASSERT_TRUE(in.good());
		std::string data;
		in >> data;
		EXPECT_EQ(data, "abcdef");
	}
	std::error_code ec;
	std::filesystem::remove(file, ec);
}

}  // namespace
