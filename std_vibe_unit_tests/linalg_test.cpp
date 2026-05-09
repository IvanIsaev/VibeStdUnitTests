#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<linalg>)
#include <linalg>
#define VIBE_HAS_LINALG 1
#else
#define VIBE_HAS_LINALG 0
#endif

#include <array>

namespace {

TEST(LinalgHeader, HeaderAvailabilityAndFeatureMacro)
{
	// <linalg> is a newer header and may not yet be implemented by all toolchains.
#if VIBE_HAS_LINALG
#ifdef __cpp_lib_linalg
	EXPECT_GE(__cpp_lib_linalg, 202311L);
#endif
#else
	GTEST_SKIP() << "<linalg> is not available in this standard library.";
#endif
}

#if VIBE_HAS_LINALG

TEST(LinalgHeader, VectorAndMatrixBasicOperations)
{
	// linalg algorithms operate on mdspan-compatible views and extents.
	std::array<double, 3> a{ 1.0, 2.0, 3.0 };
	std::array<double, 3> b{ 4.0, 5.0, 6.0 };

	std::mdspan<double, std::extents<std::size_t, 3>> av(a.data());
	std::mdspan<double, std::extents<std::size_t, 3>> bv(b.data());
	EXPECT_DOUBLE_EQ(std::linalg::dot(av, bv), 32.0);
}

#endif

}  // namespace
