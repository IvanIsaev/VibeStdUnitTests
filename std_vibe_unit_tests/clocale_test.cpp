#include <gtest/gtest.h>

#include <clocale>
#include <cstring>

namespace {

TEST(CLocaleHeader, SetLocaleAndLocaleconvAccess)
{
	// setlocale configures the active C locale categories for libc operations.
	const char* original = std::setlocale(LC_ALL, nullptr);
	ASSERT_NE(original, nullptr);

	char originalCopy[128]{};
	std::strncpy(originalCopy, original, sizeof(originalCopy) - 1);

	const char* current = std::setlocale(LC_NUMERIC, "C");
	ASSERT_NE(current, nullptr);

	std::lconv* conv = std::localeconv();
	ASSERT_NE(conv, nullptr);
	ASSERT_NE(conv->decimal_point, nullptr);
	EXPECT_STREQ(conv->decimal_point, ".");

	// Restore locale to avoid impacting unrelated tests.
	std::setlocale(LC_ALL, originalCopy);
}

}  // namespace
