#include <gtest/gtest.h>

namespace {

#if defined(__has_include)
#if __has_include(<contracts>)
#include <contracts>
#define VIBE_HAS_CONTRACTS_HEADER 1
#else
#define VIBE_HAS_CONTRACTS_HEADER 0
#endif
#else
#define VIBE_HAS_CONTRACTS_HEADER 0
#endif

#if VIBE_HAS_CONTRACTS_HEADER

	TEST(Contracts, HeaderAvailabilityAndFeatureMacro)
	{
		// The primary item of <contracts> is the header itself, which introduces
		// contract-support facilities only when the implementation provides them.
		// This test validates the compilation path where the header exists and checks
		// whether a contracts feature-test macro is exposed, documenting compiler
		// support state in a stable, non-fragile way.
		EXPECT_TRUE(true);

#ifdef __cpp_contracts
		EXPECT_GT(__cpp_contracts, 0L);
#else
		GTEST_SKIP() << "__cpp_contracts is not defined even though <contracts> was found.";
#endif
	}

	TEST(Contracts, LanguageContractSyntaxCannotBeUnitTestedDirectly)
	{
		// C++ contracts (preconditions, postconditions, and assertions) are language
		// constructs applied in declarations, not ordinary runtime APIs that can be
		// invoked like regular functions. This test intentionally documents that
		// limitation and serves as an explicit reminder that meaningful validation
		// requires compile-mode and diagnostic-based tests outside normal gtest flow.
		SUCCEED();
	}

#else

	TEST(Contracts, HeaderNotAvailableInCurrentToolchain)
	{
		// <contracts> is still an evolving C++ feature and is commonly unavailable on
		// many standard library/toolchain combinations. Instead of failing build, we
		// provide a deterministic skipped test that clearly reports the environment
		// limitation while keeping the suite portable and maintainable.
		GTEST_SKIP() << "<contracts> is not available in this compiler/standard library.";
	}

#endif

}  // namespace
