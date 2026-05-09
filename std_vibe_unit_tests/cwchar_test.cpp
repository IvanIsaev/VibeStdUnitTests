#include <gtest/gtest.h>

#include <cwchar>

namespace {

TEST(CWcharHeader, LengthCopyCompareAndSearchFunctions)
{
	// Wide-character APIs mirror <cstring> functionality for wchar_t strings.
	const wchar_t* text = L"hello";
	EXPECT_EQ(std::wcslen(text), 5u);

	wchar_t buffer[16]{};
	std::wcscpy(buffer, L"abc");
	EXPECT_EQ(std::wcscmp(buffer, L"abc"), 0);

	std::wcscat(buffer, L"def");
	EXPECT_EQ(std::wcscmp(buffer, L"abcdef"), 0);

	const wchar_t* found = std::wcschr(buffer, L'd');
	ASSERT_NE(found, nullptr);
	EXPECT_EQ(*found, L'd');
}

TEST(CWcharHeader, MemoryAndNumericConversionFunctions)
{
	// Wide-memory and numeric parsing functions support low-level processing.
	wchar_t src[4] = { L'a', L'b', L'c', L'\0' };
	wchar_t dst[4] = {};
	std::wmemcpy(dst, src, 4);
	EXPECT_EQ(std::wmemcmp(dst, src, 4), 0);

	const wchar_t* number = L"123";
	wchar_t* end = nullptr;
	const long value = std::wcstol(number, &end, 10);
	EXPECT_EQ(value, 123);
	EXPECT_EQ(*end, L'\0');
}

}  // namespace
