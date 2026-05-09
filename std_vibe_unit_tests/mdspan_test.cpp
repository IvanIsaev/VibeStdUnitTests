#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<mdspan>)
#include <mdspan>
#define VIBE_HAS_MDSPAN 1
#else
#define VIBE_HAS_MDSPAN 0
#endif

#include <array>
#include <vector>

namespace {

TEST(MdspanHeader, HeaderAvailabilityAndFeatureMacro)
{
	// This test indicates whether <mdspan> is provided by the current library.
#if VIBE_HAS_MDSPAN
#ifdef __cpp_lib_mdspan
	EXPECT_GE(__cpp_lib_mdspan, 202207L);
#endif
#else
	GTEST_SKIP() << "<mdspan> is not available in this standard library.";
#endif
}

#if VIBE_HAS_MDSPAN

TEST(MdspanHeader, DynamicExtentsAndIndexing)
{
	// mdspan maps multidimensional indices onto underlying contiguous storage.
	std::vector<int> storage(6);
	for (int i = 0; i < 6; ++i)
	{
		storage[i] = i + 1;
	}

	std::mdspan<int, std::dextents<std::size_t, 2>> view(storage.data(), 2, 3);
	EXPECT_EQ(view.extent(0), 2u);
	EXPECT_EQ(view.extent(1), 3u);
	EXPECT_EQ(view(0, 0), 1);
	EXPECT_EQ(view(1, 2), 6);
}

TEST(MdspanHeader, StaticExtentsAndRankQueries)
{
	// extents encode compile-time dimensions and expose rank/rank_dynamic traits.
	using E = std::extents<std::size_t, 2, 3>;
	EXPECT_EQ(E::rank(), 2u);
	EXPECT_EQ(E::rank_dynamic(), 0u);
	EXPECT_EQ(E::static_extent(0), 2u);
	EXPECT_EQ(E::static_extent(1), 3u);

	std::array<int, 6> data{ 1, 2, 3, 4, 5, 6 };
	std::mdspan<int, E> view(data.data());
	EXPECT_EQ(view(1, 1), 5);
}

TEST(MdspanHeader, LayoutPoliciesAndAccessorBasics)
{
	// mdspan separates layout mapping and accessor policy from element type.
	std::array<int, 6> data{ 1, 2, 3, 4, 5, 6 };

	using Ext = std::extents<std::size_t, 2, 3>;
	std::mdspan<int, Ext, std::layout_right> rowMajor(data.data());
	EXPECT_EQ(rowMajor(1, 2), 6);

	std::mdspan<int, Ext, std::layout_left> colMajor(data.data());
	EXPECT_EQ(colMajor(0, 1), data[2]);
	EXPECT_EQ(colMajor(1, 0), data[1]);
}

TEST(MdspanHeader, SubmdspanWhenAvailable)
{
	// submdspan provides slicing over existing mdspan mappings in C++26.
#ifdef __cpp_lib_submdspan
	std::array<int, 12> data{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
	std::mdspan<int, std::extents<std::size_t, 3, 4>> view(data.data());
	auto row1 = std::submdspan(view, 1, std::full_extent);
	EXPECT_EQ(row1.extent(0), 4u);
	EXPECT_EQ(row1(0), 4);
	EXPECT_EQ(row1(3), 7);
#endif
}

#endif

}  // namespace
#include <gtest/gtest.h>

#include <array>
#include <cstddef>

#if defined(__has_include)
#if __has_include(<mdspan>)
#include <mdspan>
#define VIBE_HAS_MDSPAN 1
#else
#define VIBE_HAS_MDSPAN 0
#endif
#else
#define VIBE_HAS_MDSPAN 0
#endif

namespace {

	TEST(MdspanHeader, AvailabilityAndFeatureMacro)
	{
#if VIBE_HAS_MDSPAN
#ifdef __cpp_lib_mdspan
		EXPECT_GE(__cpp_lib_mdspan, 202207L);
#else
		FAIL() << "<mdspan> present but __cpp_lib_mdspan missing.";
#endif
#else
		GTEST_SKIP() << "<mdspan> unavailable on this toolchain.";
#endif
	}

#if VIBE_HAS_MDSPAN
	TEST(MdspanHeader, ExtentsAndStaticDynamicProperties)
	{
		using E = std::extents<std::size_t, 2, std::dynamic_extent>;
		E ext(3);
		EXPECT_EQ(ext.rank(), 2u);
		EXPECT_EQ(ext.rank_dynamic(), 1u);
		EXPECT_EQ(ext.extent(0), 2u);
		EXPECT_EQ(ext.extent(1), 3u);
	}

	TEST(MdspanHeader, BasicIndexingAndLayoutRightMapping)
	{
		std::array<int, 6> data{ 1, 2, 3, 4, 5, 6 };
		using ext_t = std::extents<std::size_t, 2, 3>;
		std::mdspan<int, ext_t> m(data.data());
		EXPECT_EQ(m.rank(), 2u);
		EXPECT_EQ(m.extent(0), 2u);
		EXPECT_EQ(m.extent(1), 3u);
		EXPECT_EQ(m(0, 0), 1);
		EXPECT_EQ(m(1, 2), 6);

		m(1, 1) = 42;
		EXPECT_EQ(data[4], 42);
	}

	TEST(MdspanHeader, AccessorAndDataHandleObservers)
	{
		std::array<double, 4> data{ 0.5, 1.5, 2.5, 3.5 };
		std::mdspan<double, std::extents<std::size_t, 4>> m(data.data());
		EXPECT_EQ(m.data_handle(), data.data());
		EXPECT_EQ(m.size(), 4u);
		EXPECT_FALSE(m.empty());
		EXPECT_DOUBLE_EQ(m[2], 2.5);
	}
#endif

}  // namespace
