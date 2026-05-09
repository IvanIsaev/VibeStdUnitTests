#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<generator>)
#include <generator>
#define VIBE_HAS_GENERATOR 1
#else
#define VIBE_HAS_GENERATOR 0
#endif

#include <vector>

namespace {

TEST(GeneratorHeader, HeaderAvailabilityAndFeatureMacro)
{
	// This test reports whether the toolchain exposes the C++23/26 generator API.
#if VIBE_HAS_GENERATOR
#ifdef __cpp_lib_generator
	EXPECT_GE(__cpp_lib_generator, 202207L);
#endif
#else
	GTEST_SKIP() << "<generator> is not available in this standard library.";
#endif
}

#if VIBE_HAS_GENERATOR

static std::generator<int> CountFromTo(int first, int last)
{
	// co_yield emits each value lazily when the generator is iterated.
	for (int i = first; i <= last; ++i)
	{
		co_yield i;
	}
}

TEST(GeneratorHeader, BasicCoroutineGenerationAndIteration)
{
	// generator models an input range that resumes coroutine execution on demand.
	std::vector<int> values;
	for (int v : CountFromTo(1, 4))
	{
		values.push_back(v);
	}
	EXPECT_EQ(values, (std::vector<int>{ 1, 2, 3, 4 }));
}

TEST(GeneratorHeader, SinglePassIterationSemantics)
{
	// generator iterators are input iterators and represent single-pass traversal.
	auto g = CountFromTo(3, 5);
	auto it = g.begin();
	ASSERT_NE(it, g.end());
	EXPECT_EQ(*it, 3);
	++it;
	EXPECT_EQ(*it, 4);
	++it;
	EXPECT_EQ(*it, 5);
	++it;
	EXPECT_EQ(it, g.end());
}

#endif

}  // namespace
