#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<syncstream>)
#include <syncstream>
#define VIBE_HAS_SYNCSTREAM 1
#else
#define VIBE_HAS_SYNCSTREAM 0
#endif

#include <sstream>
#include <string>

namespace {

TEST(SyncstreamHeader, HeaderAvailabilityAndFeatureMacro)
{
	// C++20 <syncstream> wraps another stream buffer with a mutex for thread-safe output.
#if VIBE_HAS_SYNCSTREAM
#ifdef __cpp_lib_syncstream
	EXPECT_GE(__cpp_lib_syncstream, 201803L);
#endif
#else
	GTEST_SKIP() << "<syncstream> is not available in this standard library.";
#endif
}

#if VIBE_HAS_SYNCSTREAM

TEST(SyncstreamHeader, OsyncstreamEmitsToWrappedBuffer)
{
	// basic_osyncstream ties to a target stream buffer and flushes on destruction.
	std::ostringstream target;
	{
		std::osyncstream synced(target);
		synced << "line";
	}
	EXPECT_EQ(target.str(), "line");
}

TEST(SyncstreamHeader, SyncbufWrapsExternalStreamBuffer)
{
	// basic_syncbuf can be reused and manually emits/locks around the wrapped buffer.
	std::ostringstream target;
	std::syncbuf buf(target.rdbuf());
	std::ostream out(&buf);
	out << "x";
	buf.emit();
	EXPECT_EQ(target.str(), "x");
}

#endif

}  // namespace
