#include <gtest/gtest.h>

#include <cstdint>
#include <source_location>
#include <string_view>
#include <type_traits>

namespace {

	constexpr std::source_location CaptureViaDefaultArgument(
		const std::source_location location = std::source_location::current()) noexcept
	{
		return location;
	}

	TEST(SourceLocation, TypePropertiesSupportCheapValueSemantics)
	{
		// std::source_location is designed as a small value-type token describing a
		// call site. It should be trivially copyable/movable and usable in noexcept
		// contexts so it can be passed through logging/assertion APIs efficiently.
		EXPECT_TRUE((std::is_copy_constructible_v<std::source_location>));
		EXPECT_TRUE((std::is_copy_assignable_v<std::source_location>));
		EXPECT_TRUE((std::is_move_constructible_v<std::source_location>));
		EXPECT_TRUE((std::is_move_assignable_v<std::source_location>));
		EXPECT_TRUE((std::is_nothrow_move_constructible_v<std::source_location>));
		EXPECT_TRUE((std::is_nothrow_move_assignable_v<std::source_location>));
	}

	TEST(SourceLocation, CurrentCapturesCallSiteLineAndColumn)
	{
		// source_location::current() captures the immediate call site. This test
		// checks that line and column are positive and that line corresponds to the
		// expected invocation line within a one-line tolerance for tooling variance.
		const std::uint_least32_t expectedLine = static_cast<std::uint_least32_t>(__LINE__ + 1);
		const std::source_location location = std::source_location::current();

		EXPECT_GE(location.line(), 1u);
		EXPECT_GE(location.column(), 1u);
		EXPECT_TRUE(location.line() == expectedLine || location.line() == expectedLine + 1);
	}

	TEST(SourceLocation, FileNameContainsThisTranslationUnitName)
	{
		// file_name() returns an implementation-defined C string representing source
		// path at the capture site. We validate non-empty data and that it references
		// this test translation unit filename.
		const std::source_location location = std::source_location::current();
		const std::string_view fileName = location.file_name();

		EXPECT_FALSE(fileName.empty());
		EXPECT_NE(fileName.find("source_location_test.cpp"), std::string_view::npos);
	}

	TEST(SourceLocation, FunctionNameReportsCapturingFunctionContext)
	{
		// function_name() exposes an implementation-defined function signature/name
		// for the capture context. We check it is non-empty and references this test
		// case name fragment so diagnostics can attribute the right call frame.
		const std::source_location location = std::source_location::current();
		const std::string_view functionName = location.function_name();

		EXPECT_FALSE(functionName.empty());
		EXPECT_NE(functionName.find("FunctionNameReportsCapturingFunctionContext"), std::string_view::npos);
	}

	TEST(SourceLocation, DefaultArgumentPatternCapturesCallerInsteadOfCallee)
	{
		// The common API pattern is a default parameter initialized with
		// source_location::current(), which captures the caller's location rather than
		// the helper function body. This verifies caller-side line/file propagation.
		const std::uint_least32_t expectedCallerLine = static_cast<std::uint_least32_t>(__LINE__ + 1);
		const std::source_location forwarded = CaptureViaDefaultArgument();

		EXPECT_TRUE(forwarded.line() == expectedCallerLine || forwarded.line() == expectedCallerLine + 1);
		EXPECT_NE(std::string_view(forwarded.file_name()).find("source_location_test.cpp"), std::string_view::npos);
	}

	TEST(SourceLocation, ExplicitForwardingPreservesProvidedLocationObject)
	{
		// Passing an explicit source_location object to a helper should preserve the
		// original metadata exactly, which is useful for wrapper layers that forward
		// diagnostics without altering source attribution.
		const std::source_location original = std::source_location::current();
		const std::source_location forwarded = CaptureViaDefaultArgument(original);

		EXPECT_EQ(forwarded.line(), original.line());
		EXPECT_EQ(forwarded.column(), original.column());
		EXPECT_STREQ(forwarded.file_name(), original.file_name());
		EXPECT_STREQ(forwarded.function_name(), original.function_name());
	}

}  // namespace
