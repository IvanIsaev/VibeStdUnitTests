#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<text_encoding>)
#include <text_encoding>
#define VIBE_HAS_TEXT_ENCODING 1
#else
#define VIBE_HAS_TEXT_ENCODING 0
#endif

namespace {

TEST(TextEncodingHeader, HeaderAvailabilityAndFeatureMacro)
{
	// <text_encoding> is a new standard facility and may not be implemented yet.
#if VIBE_HAS_TEXT_ENCODING
#ifdef __cpp_lib_text_encoding
	EXPECT_GE(__cpp_lib_text_encoding, 202306L);
#endif
#else
	GTEST_SKIP() << "<text_encoding> is not available in this standard library.";
#endif
}

#if VIBE_HAS_TEXT_ENCODING

TEST(TextEncodingHeader, LiteralAndMibBasedConstruction)
{
	// text_encoding can identify known encodings by literal name or MIB enum.
	const std::text_encoding utf8 = std::text_encoding::literal();
	EXPECT_FALSE(utf8.name().empty());

	const std::text_encoding byMib(std::text_encoding::id::utf8);
	EXPECT_EQ(byMib.mib(), std::text_encoding::id::utf8);
}

TEST(TextEncodingHeader, EqualityAndPropertyQueries)
{
	// Equality and metadata queries expose encoding identity and aliases.
	const std::text_encoding a(std::text_encoding::id::utf8);
	const std::text_encoding b(std::text_encoding::id::utf8);
	EXPECT_EQ(a, b);
	EXPECT_FALSE(a.name().empty());
	EXPECT_FALSE(a.aliases().empty());
}

#endif

}  // namespace
