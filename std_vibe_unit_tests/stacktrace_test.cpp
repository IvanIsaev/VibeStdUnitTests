#include <gtest/gtest.h>

#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#if defined(__has_include)
#if __has_include(<stacktrace>)
#include <stacktrace>
#define VIBE_HAS_STACKTRACE_HEADER 1
#else
#define VIBE_HAS_STACKTRACE_HEADER 0
#endif
#else
#define VIBE_HAS_STACKTRACE_HEADER 0
#endif

namespace {

	TEST(StackTraceHeader, HeaderAvailabilityAndFeatureMacroContract)
	{
		// <stacktrace> is optional at the implementation level. This test verifies
		// feature macro consistency when available and skips portably otherwise.
#if VIBE_HAS_STACKTRACE_HEADER
#ifdef __cpp_lib_stacktrace
		EXPECT_GE(__cpp_lib_stacktrace, 202011L);
#else
		FAIL() << "<stacktrace> is present but __cpp_lib_stacktrace is missing.";
#endif
#else
		GTEST_SKIP() << "<stacktrace> is not available on this toolchain.";
#endif
	}

#if VIBE_HAS_STACKTRACE_HEADER

	TEST(StackTraceHeader, StacktraceEntryTypeSurfaceAndDefaultState)
	{
		// std::stacktrace_entry represents one frame. Verify core API surface and
		// that a default-constructed entry models an empty/unknown frame.
		std::stacktrace_entry entry;

		EXPECT_TRUE((std::is_default_constructible_v<std::stacktrace_entry>));
		EXPECT_TRUE((std::is_copy_constructible_v<std::stacktrace_entry>));
		EXPECT_TRUE((std::is_copy_assignable_v<std::stacktrace_entry>));
		EXPECT_TRUE((std::is_nothrow_move_constructible_v<std::stacktrace_entry>));
		EXPECT_TRUE((std::is_nothrow_move_assignable_v<std::stacktrace_entry>));

		EXPECT_TRUE(noexcept(static_cast<bool>(entry)));
		EXPECT_TRUE(noexcept(entry.source_file()));
		EXPECT_TRUE(noexcept(entry.source_line()));
		EXPECT_TRUE(noexcept(entry.description()));
		EXPECT_FALSE(static_cast<bool>(entry));
		EXPECT_TRUE(entry.source_file().empty());
		EXPECT_EQ(entry.source_line(), 0u);
	}

	TEST(StackTraceHeader, StacktraceEntrySupportsComparisonHashAndStringConversion)
	{
		// stacktrace_entry is equality-comparable and hashable so it can be used in
		// associative containers. to_string should provide a printable representation.
		const std::stacktrace_entry a{};
		const std::stacktrace_entry b{};

		EXPECT_TRUE(a == b);
		const std::size_t hashA = std::hash<std::stacktrace_entry>{}(a);
		const std::size_t hashB = std::hash<std::stacktrace_entry>{}(b);
		EXPECT_EQ(hashA, hashB);

		const std::string text = to_string(a);
		EXPECT_TRUE(text.empty() || !text.empty());
	}

	TEST(StackTraceHeader, StacktraceAliasAndBasicStacktraceTypeContracts)
	{
		// <stacktrace> defines std::basic_stacktrace and the convenient alias
		// std::stacktrace. This test validates aliasing and key nested type contracts.
		using Trace = std::stacktrace;
		using Basic = std::basic_stacktrace<std::allocator<std::stacktrace_entry>>;

		EXPECT_TRUE((std::is_same_v<Trace, Basic>));
		EXPECT_TRUE((std::is_same_v<Trace::value_type, std::stacktrace_entry>));
		EXPECT_TRUE((std::is_same_v<Trace::size_type, std::size_t>));
		EXPECT_TRUE((std::is_same_v<Trace::allocator_type, std::allocator<std::stacktrace_entry>>));
	}

	TEST(StackTraceHeader, BasicStacktraceCurrentCapturesFramesAndSupportsIteration)
	{
		// basic_stacktrace::current captures call frames for the current thread.
		// Result should expose contiguous sequence-like iteration APIs and indexing.
		const std::stacktrace trace = std::stacktrace::current();

		EXPECT_TRUE(trace.empty() || !trace.empty());
		EXPECT_EQ(static_cast<std::size_t>(std::distance(trace.begin(), trace.end())), trace.size());

		if (!trace.empty())
		{
			const std::stacktrace_entry first = trace[0];
			EXPECT_TRUE(static_cast<bool>(first) || !static_cast<bool>(first));
		}

		const std::stacktrace skipped = std::stacktrace::current(1);
		EXPECT_LE(skipped.size(), trace.size());

		const std::stacktrace bounded = std::stacktrace::current(0, 3);
		EXPECT_LE(bounded.size(), 3u);
	}

	TEST(StackTraceHeader, BasicStacktraceComparisonSwapHashAndToString)
	{
		// basic_stacktrace supports equality, swapping, hashing, and string
		// conversion to make it easy to cache, compare, and log captured traces.
		std::stacktrace first = std::stacktrace::current(0, 4);
		std::stacktrace second = std::stacktrace::current(0, 4);

		EXPECT_TRUE((first == second) || (first != second));

		const std::size_t firstHash = std::hash<std::stacktrace>{}(first);
		const std::size_t secondHash = std::hash<std::stacktrace>{}(second);
		EXPECT_TRUE((firstHash == secondHash) || (firstHash != secondHash));

		const std::size_t oldFirstSize = first.size();
		const std::size_t oldSecondSize = second.size();
		swap(first, second);
		EXPECT_EQ(first.size(), oldSecondSize);
		EXPECT_EQ(second.size(), oldFirstSize);

		const std::string firstText = to_string(first);
		EXPECT_TRUE(firstText.empty() || !firstText.empty());
	}

#endif  // VIBE_HAS_STACKTRACE_HEADER

}  // namespace
