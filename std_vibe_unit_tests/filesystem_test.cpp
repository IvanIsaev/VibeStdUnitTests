#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace {

TEST(FilesystemHeader, PathCompositionAndInspection)
{
	// std::filesystem::path models platform-native path syntax and decomposition.
	const std::filesystem::path p = std::filesystem::path("root") / "child" / "file.txt";
	EXPECT_EQ(p.filename().string(), "file.txt");
	EXPECT_EQ(p.extension().string(), ".txt");
	EXPECT_EQ(p.parent_path().filename().string(), "child");
}

TEST(FilesystemHeader, ExistsCreateDirectoriesAndRemoval)
{
	// Filesystem utilities can safely create and remove temporary directory trees.
	const auto base = std::filesystem::temp_directory_path() / "vibe_fs_test_dir";
	const auto nested = base / "a" / "b";

	std::error_code ec;
	std::filesystem::remove_all(base, ec);
	ec.clear();

	EXPECT_TRUE(std::filesystem::create_directories(nested, ec));
	EXPECT_FALSE(ec);
	EXPECT_TRUE(std::filesystem::exists(nested));
	EXPECT_TRUE(std::filesystem::is_directory(nested));

	const auto removed = std::filesystem::remove_all(base, ec);
	EXPECT_FALSE(ec);
	EXPECT_GT(removed, 0u);
}

TEST(FilesystemHeader, FileCopyRenameStatusAndSpaceQueries)
{
	// File operations include copy/rename/status checks and disk-space queries.
	const auto base = std::filesystem::temp_directory_path() / "vibe_fs_test_files";
	const auto src = base / "source.txt";
	const auto dst = base / "copy.txt";
	const auto renamed = base / "renamed.txt";

	std::error_code ec;
	std::filesystem::remove_all(base, ec);
	ec.clear();
	EXPECT_TRUE(std::filesystem::create_directories(base, ec));
	ASSERT_FALSE(ec);

	{
		std::ofstream out(src.string());
		ASSERT_TRUE(out.good());
		out << "payload";
	}
	EXPECT_TRUE(std::filesystem::exists(src));

	std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
	EXPECT_FALSE(ec);
	EXPECT_TRUE(std::filesystem::exists(dst));

	std::filesystem::rename(dst, renamed, ec);
	EXPECT_FALSE(ec);
	EXPECT_TRUE(std::filesystem::exists(renamed));

	const auto st = std::filesystem::status(renamed, ec);
	EXPECT_FALSE(ec);
	EXPECT_EQ(st.type(), std::filesystem::file_type::regular);

	const auto info = std::filesystem::space(base, ec);
	EXPECT_FALSE(ec);
	EXPECT_GT(info.capacity, 0u);

	std::filesystem::remove_all(base, ec);
	EXPECT_FALSE(ec);
}

}  // namespace
