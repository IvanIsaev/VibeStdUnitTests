#include <gtest/gtest.h>

#include <complex>

namespace {

TEST(ComplexHeader, ConstructionAndComponentAccess)
{
	// std::complex stores and exposes real/imaginary components.
	std::complex<double> z(3.0, 4.0);
	EXPECT_DOUBLE_EQ(z.real(), 3.0);
	EXPECT_DOUBLE_EQ(z.imag(), 4.0);

	z.real(5.0);
	z.imag(6.0);
	EXPECT_EQ(z, std::complex<double>(5.0, 6.0));
}

TEST(ComplexHeader, ArithmeticOperationsAndConjugate)
{
	// Arithmetic follows complex algebra rules.
	const std::complex<double> a(1.0, 2.0);
	const std::complex<double> b(3.0, -1.0);
	EXPECT_EQ(a + b, std::complex<double>(4.0, 1.0));
	EXPECT_EQ(a - b, std::complex<double>(-2.0, 3.0));
	EXPECT_EQ(a * b, std::complex<double>(5.0, 5.0));
	EXPECT_EQ(std::conj(a), std::complex<double>(1.0, -2.0));
}

TEST(ComplexHeader, MagnitudePhaseAndPolarFunctions)
{
	// abs/norm/arg/polar interconvert geometric complex representations.
	const std::complex<double> z(3.0, 4.0);
	EXPECT_DOUBLE_EQ(std::abs(z), 5.0);
	EXPECT_DOUBLE_EQ(std::norm(z), 25.0);
	EXPECT_NEAR(std::arg(std::complex<double>(0.0, 1.0)), 1.57079632679, 1e-9);

	const auto p = std::polar(2.0, 0.0);
	EXPECT_NEAR(p.real(), 2.0, 1e-12);
	EXPECT_NEAR(p.imag(), 0.0, 1e-12);
}

TEST(ComplexHeader, ElementaryComplexFunctions)
{
	// <complex> overloads transcendental functions for complex domains.
	const std::complex<double> z(1.0, 1.0);
	const auto ez = std::exp(z);
	EXPECT_NEAR(ez.real(), 1.4686939399, 1e-9);
	EXPECT_NEAR(ez.imag(), 2.2873552871, 1e-9);

	const auto sz = std::sin(z);
	EXPECT_NEAR(sz.real(), 1.2984575814, 1e-9);
	EXPECT_NEAR(sz.imag(), 0.6349639148, 1e-9);
}

}  // namespace
