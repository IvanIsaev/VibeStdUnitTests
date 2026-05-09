#include <gtest/gtest.h>

#include <type_traits>

#if defined(__has_include)
#if __has_include(<stdfloat>)
#include <stdfloat>
#define VIBE_HAS_STDFLOAT_HEADER 1
#else
#define VIBE_HAS_STDFLOAT_HEADER 0
#endif
#else
#define VIBE_HAS_STDFLOAT_HEADER 0
#endif

namespace {

	TEST(StdFloat, HeaderAvailabilityAndFeatureMacroContract)
	{
		// <stdfloat> is optional in practice because implementation support for the
		// underlying extended floating types can vary by compiler/standard library.
		// This test records whether the header is available and validates the feature
		// macro contract when it is present.
#if VIBE_HAS_STDFLOAT_HEADER
		EXPECT_GE(__cpp_lib_stdfloat, 202311L);
#else
		GTEST_SKIP() << "<stdfloat> is not available on this toolchain.";
#endif
	}

#if VIBE_HAS_STDFLOAT_HEADER

	TEST(StdFloat, Float16AliasWhenSupported)
	{
		// std::float16_t is provided only when the implementation defines
		// __STDCPP_FLOAT16_T__. When present, it is a distinct floating-point type
		// intended for 16-bit interchange/compute scenarios.
#ifdef __STDCPP_FLOAT16_T__
		EXPECT_TRUE((std::is_floating_point_v<std::float16_t>));
		EXPECT_EQ(sizeof(std::float16_t), 2u);
#else
		GTEST_SKIP() << "std::float16_t is not provided by this implementation.";
#endif
	}

	TEST(StdFloat, Float32AliasWhenSupported)
	{
		// std::float32_t maps to a floating type with exactly 32 storage bits when
		// __STDCPP_FLOAT32_T__ is defined. This is commonly IEEE binary32.
#ifdef __STDCPP_FLOAT32_T__
		EXPECT_TRUE((std::is_floating_point_v<std::float32_t>));
		EXPECT_EQ(sizeof(std::float32_t), 4u);
#else
		GTEST_SKIP() << "std::float32_t is not provided by this implementation.";
#endif
	}

	TEST(StdFloat, Float64AliasWhenSupported)
	{
		// std::float64_t maps to a floating type with exactly 64 storage bits when
		// __STDCPP_FLOAT64_T__ is defined. This is commonly IEEE binary64.
#ifdef __STDCPP_FLOAT64_T__
		EXPECT_TRUE((std::is_floating_point_v<std::float64_t>));
		EXPECT_EQ(sizeof(std::float64_t), 8u);
#else
		GTEST_SKIP() << "std::float64_t is not provided by this implementation.";
#endif
	}

	TEST(StdFloat, Float128AliasWhenSupported)
	{
		// std::float128_t is available only when __STDCPP_FLOAT128_T__ is defined.
		// Platforms that support it usually expose 128-bit binary floating storage.
#ifdef __STDCPP_FLOAT128_T__
		EXPECT_TRUE((std::is_floating_point_v<std::float128_t>));
		EXPECT_EQ(sizeof(std::float128_t), 16u);
#else
		GTEST_SKIP() << "std::float128_t is not provided by this implementation.";
#endif
	}

	TEST(StdFloat, BFloat16AliasWhenSupported)
	{
		// std::bfloat16_t is available when __STDCPP_BFLOAT16_T__ is defined and is
		// typically used for ML-centric reduced-precision arithmetic.
#ifdef __STDCPP_BFLOAT16_T__
		EXPECT_TRUE((std::is_floating_point_v<std::bfloat16_t>));
		EXPECT_EQ(sizeof(std::bfloat16_t), 2u);
#else
		GTEST_SKIP() << "std::bfloat16_t is not provided by this implementation.";
#endif
	}

#endif  // VIBE_HAS_STDFLOAT_HEADER

}  // namespace
