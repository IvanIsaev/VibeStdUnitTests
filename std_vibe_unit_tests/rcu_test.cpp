#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<rcu>)
#include <rcu>
#define VIBE_HAS_RCU 1
#else
#define VIBE_HAS_RCU 0
#endif

namespace {

TEST(RcuHeader, HeaderAvailabilityAndFeatureMacro)
{
	// RCU enables readers to proceed without locks while writers defer reclamation.
#if VIBE_HAS_RCU
#ifdef __cpp_lib_rcu
	EXPECT_GE(__cpp_lib_rcu, 202306L);
#endif
#else
	GTEST_SKIP() << "<rcu> is not available in this standard library.";
#endif
}

#if VIBE_HAS_RCU

TEST(RcuHeader, DefaultDomainLockUnlock)
{
	// rcu_default_domain() provides a process-wide RCU domain for read-side sections.
	std::rcu_domain& domain = std::rcu_default_domain();
	domain.lock();
	domain.unlock();
}

TEST(RcuHeader, RcuSynchronizeAfterReadSideCriticalSection)
{
	// rcu_synchronize waits until no reader holds protection on the domain.
	std::rcu_domain& domain = std::rcu_default_domain();
	domain.lock();
	domain.unlock();
	std::rcu_synchronize(domain);
}

#endif

}  // namespace
