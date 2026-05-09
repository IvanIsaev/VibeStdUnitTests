#include <gtest/gtest.h>

#if defined(__has_include) && __has_include(<hazard_pointer>)
#include <hazard_pointer>
#define VIBE_HAS_HAZARD_POINTER 1
#else
#define VIBE_HAS_HAZARD_POINTER 0
#endif

#include <atomic>

namespace {

TEST(HazardPointerHeader, HeaderAvailabilityAndFeatureMacro)
{
	// Hazard pointers defer reclamation until no concurrent readers hold protection.
#if VIBE_HAS_HAZARD_POINTER
#ifdef __cpp_lib_hazard_pointer
	EXPECT_GE(__cpp_lib_hazard_pointer, 202306L);
#endif
#else
	GTEST_SKIP() << "<hazard_pointer> is not available in this standard library.";
#endif
}

#if VIBE_HAS_HAZARD_POINTER

TEST(HazardPointerHeader, EmptyVsNonemptyHazardPointer)
{
	// Default-constructed hazard_pointer is empty; make_hazard_pointer() is nonempty.
	std::hazard_pointer empty{};
	EXPECT_TRUE(empty.empty());

	std::hazard_pointer hp = std::make_hazard_pointer();
	EXPECT_FALSE(hp.empty());
}

TEST(HazardPointerHeader, ProtectLoadsAtomicPointer)
{
	// protect reads the atomic and registers the address as hazard-protected.
	struct Widget
	{
		int value{ 99 };
	};

	Widget w{};
	std::atomic<Widget*> slot{ &w };
	std::hazard_pointer hp = std::make_hazard_pointer();
	Widget* p = hp.protect(slot);
	ASSERT_NE(p, nullptr);
	EXPECT_EQ(p->value, 99);
	hp.reset_protection();
}

TEST(HazardPointerHeader, TryProtectValidatesConsistentLoad)
{
	// try_protect confirms the loaded pointer still matches the atomic (ABA-safe path).
	int x = 1;
	std::atomic<int*> a{ &x };
	std::hazard_pointer hp = std::make_hazard_pointer();
	int* p = nullptr;
	EXPECT_TRUE(hp.try_protect(p, a));
	EXPECT_EQ(p, &x);
	hp.reset_protection();
}

#endif

}  // namespace
