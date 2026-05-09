#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<simd>)
#include <simd>
#define VIBE_HAS_SIMD 1
#else
#define VIBE_HAS_SIMD 0
#endif

namespace {

TEST(SimdHeader, HeaderAvailabilityAndFeatureMacro)
{
	// <simd> availability varies by standard library and compiler support level.
#if VIBE_HAS_SIMD
#ifdef __cpp_lib_simd
	EXPECT_GE(__cpp_lib_simd, 202207L);
#endif
#else
	GTEST_SKIP() << "<simd> is not available in this standard library.";
#endif
}

#if VIBE_HAS_SIMD

TEST(SimdHeader, BasicConstructionArithmeticAndReduction)
{
	// basic_simd provides vectorized arithmetic over implementation-defined lanes.
	using simd_t = std::simd<float>;
	simd_t a(1.0f);
	simd_t b(2.0f);
	simd_t c = a + b;
	EXPECT_EQ(std::reduce(c), static_cast<float>(c.size()) * 3.0f);
}

TEST(SimdHeader, MaskOperations)
{
	// simd_mask represents per-lane boolean conditions.
	using simd_t = std::simd<int>;
	simd_t values(5);
	auto mask = (values == simd_t(5));
	EXPECT_EQ(std::reduce(mask), static_cast<int>(values.size()));
}

#endif

}  // namespace
