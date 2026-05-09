#include <gtest/gtest.h>

#include <numeric>
#include <random>
#include <vector>

namespace {

TEST(RandomHeader, EnginesSeedAndDeterminism)
{
	// Pseudo-random engines are deterministic given equal seed/state.
	std::mt19937 a(1234);
	std::mt19937 b(1234);
	EXPECT_EQ(a(), b());
	EXPECT_EQ(a(), b());
	EXPECT_EQ(a(), b());
}

TEST(RandomHeader, IntegerAndRealDistributions)
{
	// Distributions map uniform engine bits into target statistical domains.
	std::mt19937 eng(42);
	std::uniform_int_distribution<int> distInt(1, 6);
	std::uniform_real_distribution<double> distReal(0.0, 1.0);

	for (int i = 0; i < 128; ++i)
	{
		const int v = distInt(eng);
		EXPECT_GE(v, 1);
		EXPECT_LE(v, 6);

		const double r = distReal(eng);
		EXPECT_GE(r, 0.0);
		EXPECT_LT(r, 1.0);
	}
}

TEST(RandomHeader, BernoulliNormalAndDiscreteDistributions)
{
	// Many distribution families model distinct real-world stochastic processes.
	std::mt19937 eng(9);
	std::bernoulli_distribution coin(0.25);
	std::normal_distribution<double> normal(10.0, 2.0);
	std::discrete_distribution<int> discrete{ 1.0, 3.0, 6.0 };

	const bool b = coin(eng);
	(void)b;
	const double n = normal(eng);
	EXPECT_TRUE(std::isfinite(n));
	const int d = discrete(eng);
	EXPECT_GE(d, 0);
	EXPECT_LE(d, 2);
}

TEST(RandomHeader, ShuffleAndSamplingHelpers)
{
	// shuffle uses a supplied URBG to randomize sequence order.
	std::vector<int> values(10);
	std::iota(values.begin(), values.end(), 0);
	std::vector<int> original = values;

	std::mt19937 eng(17);
	std::shuffle(values.begin(), values.end(), eng);
	EXPECT_EQ(values.size(), original.size());
	EXPECT_NE(values, original);
}

}  // namespace
