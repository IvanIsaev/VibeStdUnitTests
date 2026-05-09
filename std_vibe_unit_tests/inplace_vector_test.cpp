#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<inplace_vector>)
#include <inplace_vector>
#define VIBE_HAS_INPLACE_VECTOR 1
#else
#define VIBE_HAS_INPLACE_VECTOR 0
#endif

#include <string>
#include <vector>

namespace {

TEST(InplaceVectorHeader, HeaderAvailabilityAndFeatureMacro)
{
	// This test reports whether <inplace_vector> is implemented by this library.
#if VIBE_HAS_INPLACE_VECTOR
#ifdef __cpp_lib_inplace_vector
	EXPECT_GE(__cpp_lib_inplace_vector, 0L);
#endif
#else
	GTEST_SKIP() << "<inplace_vector> is not available in this standard library.";
#endif
}

#if VIBE_HAS_INPLACE_VECTOR

TEST(InplaceVectorHeader, StaticCapacityAndCoreContainerProperties)
{
	// std::inplace_vector<T, N> has fixed compile-time capacity and variable size.
	std::inplace_vector<int, 5> v;
	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0u);
	EXPECT_EQ(v.capacity(), 5u);
	EXPECT_EQ(v.max_size(), 5u);
}

TEST(InplaceVectorHeader, PushPopAndElementAccess)
{
	// Push and pop mutate size while capacity remains fixed.
	std::inplace_vector<std::string, 4> v;
	v.push_back("a");
	v.emplace_back("b");
	EXPECT_EQ(v.front(), "a");
	EXPECT_EQ(v.back(), "b");
	EXPECT_EQ(v[1], "b");
	EXPECT_EQ(v.at(0), "a");
	EXPECT_THROW((void)v.at(2), std::out_of_range);

	v.pop_back();
	EXPECT_EQ(v.size(), 1u);
}

TEST(InplaceVectorHeader, InsertEraseAndClearModifiers)
{
	// inplace_vector supports vector-like middle insertion/erasure and clear.
	std::inplace_vector<int, 8> v{ 1, 3, 4 };
	auto it = v.insert(v.begin() + 1, 2);
	EXPECT_EQ(*it, 2);
	EXPECT_EQ((std::vector<int>(v.begin(), v.end())), (std::vector<int>{ 1, 2, 3, 4 }));

	v.insert(v.end(), { 5, 6 });
	EXPECT_EQ((std::vector<int>(v.begin(), v.end())), (std::vector<int>{ 1, 2, 3, 4, 5, 6 }));

	v.erase(v.begin());   // erase 1
	v.erase(v.begin() + 2);  // erase 4
	EXPECT_EQ((std::vector<int>(v.begin(), v.end())), (std::vector<int>{ 2, 3, 5, 6 }));

	v.clear();
	EXPECT_TRUE(v.empty());
}

TEST(InplaceVectorHeader, AssignmentResizeAndSwapOperations)
{
	// assign/resize and swap behave like fixed-capacity vector operations.
	std::inplace_vector<int, 6> a;
	a.assign(3, 9);
	EXPECT_EQ((std::vector<int>(a.begin(), a.end())), (std::vector<int>{ 9, 9, 9 }));

	a.resize(5, 7);
	EXPECT_EQ((std::vector<int>(a.begin(), a.end())), (std::vector<int>{ 9, 9, 9, 7, 7 }));
	a.resize(2);
	EXPECT_EQ((std::vector<int>(a.begin(), a.end())), (std::vector<int>{ 9, 9 }));

	std::inplace_vector<int, 6> b{ 1, 2 };
	a.swap(b);
	EXPECT_EQ((std::vector<int>(a.begin(), a.end())), (std::vector<int>{ 1, 2 }));

	using std::swap;
	swap(a, b);
	EXPECT_EQ((std::vector<int>(a.begin(), a.end())), (std::vector<int>{ 9, 9 }));
}

#endif

}  // namespace
#include <gtest/gtest.h>

#include <string>

#if defined(__has_include)
#if __has_include(<inplace_vector>)
#include <inplace_vector>
#define VIBE_HAS_INPLACE_VECTOR 1
#else
#define VIBE_HAS_INPLACE_VECTOR 0
#endif
#else
#define VIBE_HAS_INPLACE_VECTOR 0
#endif

namespace {

	TEST(InplaceVectorHeader, AvailabilityAndFeatureMacro)
	{
#if VIBE_HAS_INPLACE_VECTOR
#ifdef __cpp_lib_inplace_vector
		EXPECT_GE(__cpp_lib_inplace_vector, 202406L);
#else
		FAIL() << "<inplace_vector> exists but __cpp_lib_inplace_vector missing.";
#endif
#else
		GTEST_SKIP() << "<inplace_vector> unavailable on this toolchain.";
#endif
	}

#if VIBE_HAS_INPLACE_VECTOR
	TEST(InplaceVectorHeader, CapacityAndElementAccess)
	{
		// inplace_vector<T, N> has fixed maximum capacity known at compile time.
		std::inplace_vector<int, 4> v;
		EXPECT_TRUE(v.empty());
		EXPECT_EQ(v.capacity(), 4u);

		v.push_back(1);
		v.push_back(2);
		EXPECT_EQ(v.front(), 1);
		EXPECT_EQ(v.back(), 2);
		EXPECT_EQ(v[1], 2);
		EXPECT_EQ(v.at(0), 1);
		EXPECT_THROW((void)v.at(2), std::out_of_range);
	}

	TEST(InplaceVectorHeader, ModifiersInsertEraseResizeSwap)
	{
		std::inplace_vector<std::string, 6> v{ "a", "c" };
		v.insert(v.begin() + 1, "b");
		EXPECT_EQ(v[1], "b");

		v.emplace_back("d");
		v.erase(v.begin());
		EXPECT_EQ(v.front(), "b");

		v.resize(2);
		EXPECT_EQ(v.size(), 2u);
		v.clear();
		EXPECT_TRUE(v.empty());

		std::inplace_vector<std::string, 6> other{ "x" };
		v.swap(other);
		EXPECT_EQ(v.size(), 1u);
		EXPECT_EQ(v.front(), "x");
	}
#endif

}  // namespace
