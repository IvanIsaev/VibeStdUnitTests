#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<print>)
#include <print>
#define VIBE_HAS_PRINT 1
#else
#define VIBE_HAS_PRINT 0
#endif

#include <cstdio>
#include <string>

namespace {

TEST(PrintHeader, HeaderAvailabilityAndFeatureMacro)
{
	// <print> may be unavailable depending on standard library implementation.
#if VIBE_HAS_PRINT
#ifdef __cpp_lib_print
	EXPECT_GE(__cpp_lib_print, 202207L);
#endif
#else
	GTEST_SKIP() << "<print> is not available in this standard library.";
#endif
}

#if VIBE_HAS_PRINT

TEST(PrintHeader, PrintToFileStreamWhenAvailable)
{
	// std::print/std::println can target FILE* in addition to stdout/stderr.
	FILE* f = nullptr;
	EXPECT_EQ(tmpfile_s(&f), 0);
	ASSERT_NE(f, nullptr);

	std::print(f, "value={}", 42);
	std::println(f, " end");
	std::fflush(f);
	std::rewind(f);

	char buffer[64]{};
	const auto read = std::fread(buffer, 1, sizeof(buffer) - 1, f);
	EXPECT_GT(read, 0u);
	EXPECT_NE(std::string(buffer).find("value=42"), std::string::npos);

	std::fclose(f);
}

#endif

}  // namespace
