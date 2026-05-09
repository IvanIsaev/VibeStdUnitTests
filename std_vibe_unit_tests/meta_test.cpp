#include <gtest/gtest.h>

#if defined(__has_include)
#if __has_include(<meta>)
#include <meta>
#define VIBE_HAS_META_HEADER 1
#else
#define VIBE_HAS_META_HEADER 0
#endif
#else
#define VIBE_HAS_META_HEADER 0
#endif

namespace {

	TEST(MetaHeader, HeaderAvailabilityAndFeatureMacroContract)
	{
		// <meta> is a bleeding-edge reflection header and may not yet be shipped by
		// many standard library/toolchain combinations. This test keeps the suite
		// portable while verifying that basic feature-test plumbing is coherent when
		// support is present.
#if VIBE_HAS_META_HEADER
#ifdef __cpp_lib_meta
		// The exact revision may evolve; requiring at least a nonzero advertised
		// value validates that the implementation claims concrete library support.
		EXPECT_GT(__cpp_lib_meta, 0L);
#else
		FAIL() << "<meta> is present but __cpp_lib_meta is not defined.";
#endif
#else
		GTEST_SKIP() << "<meta> is not available on this toolchain.";
#endif
	}

	TEST(MetaHeader, ReflectionLibrarySurfaceIsToolchainDependent)
	{
		// Standardized reflection APIs in <meta> are still evolving rapidly across
		// implementations. This test intentionally documents availability state so
		// adding concrete per-entity checks can be done once the toolchain exposes
		// a stable and complete <meta> surface.
#if VIBE_HAS_META_HEADER
#ifdef __cpp_lib_meta
		SUCCEED() << "<meta> is available; extend this suite with concrete entities "
		             "as your compiler's reflection API stabilizes.";
#else
		SUCCEED() << "<meta> header exists but no feature macro is advertised.";
#endif
#else
		GTEST_SKIP() << "<meta> unavailable; no library entities to validate yet.";
#endif
	}

}  // namespace
