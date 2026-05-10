/*
 * =============================================================================
 * RAII in C++ — Resource Acquisition Is Initialization
 * =============================================================================
 *
 * RAII binds the lifetime of a resource (memory, file handle, lock, socket,
 * database connection, GPU buffer, etc.) to the lifetime of a C++ object.
 * You acquire the resource in the object's constructor (or in a factory that
 * immediately stores the resource inside an RAII type) and release it in the
 * destructor. When the object goes out of scope—normally, or because an
 * exception unwinds the stack—the destructor runs automatically and the
 * resource is cleaned up exactly once.
 *
 * Why it matters
 * --------------
 * Without RAII, every success path and every error path must remember to free
 * resources. That pattern is brittle: a new return statement or thrown
 * exception often becomes a leak or double-free bug. RAII centralizes cleanup
 * in one place (the destructor), so control flow becomes less error-prone and
 * exception-safe cleanup is the default.
 *
 * This is not a "Gang of Four" behavioral pattern in the classic sense; it is
 * a C++ idiom that underlies smart pointers, containers, and most standard
 * library types. Treating it as a pattern here highlights how object lifetime
 * models *behavior* (when work happens) in real programs.
 *
 * Core rule
 * ---------
 *   For every acquisition there must be exactly one owning object whose
 *   destructor releases the resource. Non-owning pointers or references may
 *   observe the resource, but they must not delete it.
 *
 * Exception safety (brief)
 * ------------------------
 * Destructors run during stack unwinding. Therefore destructors should not
 * throw exceptions (in practice: mark them noexcept and swallow or log
 * secondary errors). Leaks are avoided because unwinding still destroys local
 * objects in reverse order of construction.
 *
 * Move semantics and unique ownership
 * -----------------------------------
 * Many RAII types are move-only: ownership transfers from a source object to
 * a destination; the moved-from object is left in a valid "empty" state so its
 * destructor remains safe. std::unique_ptr and std::thread are canonical
 * examples.
 *
 * Rule of Zero / Rule of Five
 * ---------------------------
 * If you manage a raw resource directly, you often need to define or delete
 * copy constructor, copy assignment, move constructor, move assignment, and
 * destructor ("Rule of Five"). Prefer the Rule of Zero: wrap raw resources in
 * standard types (smart pointers, containers) so the compiler-generated
 * special members are correct.
 *
 * Standard library RAII you already use
 * -------------------------------------
 *   std::vector, std::string  — own contiguous storage; free on destruction
 *   std::fstream, std::ofstream — close files in destructors
 *   std::lock_guard, std::unique_lock, std::scoped_lock — release mutexes
 *   std::unique_ptr, std::shared_ptr — delete or custom-delete memory
 *   std::thread — join or detach in destructor (prefer explicit join before end of scope)
 *
 * When RAII is not enough
 * -----------------------
 * Some resources have complex sharing graphs, cycles, or asynchronous teardown.
 * You may need weak_ptr, explicit shutdown protocols, or arenas. RAII remains
 * the first tool; it composes with those designs.
 *
 * Ownership idioms (quick map)
 * ----------------------------
 *   unique ownership     → std::unique_ptr<T>, move-only handles, containers
 *   shared ownership     → std::shared_ptr<T> + std::weak_ptr to break cycles
 *   non-owning observe   → T* or T& or std::span<T> — do not release
 *   "maybe" ownership    → std::optional<std::unique_ptr<T>> or pointers with
 *                          documented null = no ownership
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   // Smart pointer: single owner; delete runs when the unique_ptr dies.
 *   {
 *     auto p = std::make_unique<std::string>("hello");
 *     use(*p);
 *   } // ~unique_ptr → delete string
 *
 *   // Container: all elements destroyed in reverse order when vector dies.
 *   {
 *     std::vector<FileTicket> files;
 *     files.emplace_back(open("a.txt"));
 *     files.emplace_back(open("b.txt"));
 *   } // each FileTicket destructor runs; vector frees storage
 *
 *   // Mutex: lock acquired in lock_guard ctor, released in dtor.
 *   void thread_safe_increment(std::mutex& m, int& x) {
 *     std::lock_guard<std::mutex> lock(m);
 *     ++x;
 *   }
 *
 *   // shared_ptr: last remaining shared_ptr destroys the object (thread-safe
 *   // refcounting). Use when lifetime is genuinely shared across components.
 *   auto a = std::make_shared<BigBuffer>();
 *   auto b = a; // same control block; one destruction when both are gone
 *
 * Common pitfalls
 * ---------------
 *   • Two owners for the same raw pointer → double free or use-after-free.
 *     Fix: one unique_ptr, or clear documentation + single delete site.
 *   • Destructor that throws → std::terminate during stack unwind if another
 *     exception is active. Fix: noexcept destructors; log instead of throw.
 *   • Manual resource release + still letting RAII object die → double close
 *     or double unlock. Fix: only release inside the RAII type, or dismiss().
 *   • std::thread joinable at destruction → std::terminate. Fix: join(),
 *     detach(), or jthread (C++20).
 *
 * Composition
 * -----------
 * RAII types nest cleanly: a struct holding std::vector<std::unique_ptr<Foo>>
 * destroys each Foo when the vector clears or the struct ends. Prefer this
 * "layered ownership" over raw pointers between components.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <exception>
#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace usage_examples::patterns::behavioral {

	// -----------------------------------------------------------------
	// Example: minimal RAII — constructor acquires, destructor releases.
	// A global counter stands in for "how many resources are outstanding."
	// -----------------------------------------------------------------
	inline int g_live_resources = 0;

	struct ResourceTicket
	{
		explicit ResourceTicket(int id) : id_(id) { ++g_live_resources; }

		ResourceTicket(const ResourceTicket&) = delete;
		ResourceTicket& operator=(const ResourceTicket&) = delete;
		ResourceTicket(ResourceTicket&& other) noexcept : id_(other.id_) { other.id_ = -1; }
		ResourceTicket& operator=(ResourceTicket&& other) noexcept
		{
			if (this != &other)
			{
				reset();
				id_ = other.id_;
				other.id_ = -1;
			}
			return *this;
		}

		~ResourceTicket() { reset(); }

		[[nodiscard]] int id() const { return id_; }
		[[nodiscard]] bool empty() const { return id_ < 0; }

	private:
		void reset()
		{
			if (id_ >= 0)
			{
				--g_live_resources;
				id_ = -1;
			}
		}

		int id_;
	};

	// -----------------------------------------------------------------
	// Example: ScopeGuard — run a function at scope exit (commit/rollback).
	// std::uncaught_exceptions() distinguishes return from unwind (C++17+).
	// -----------------------------------------------------------------
	template <typename Fn>
	class ScopeGuard
	{
	public:
		explicit ScopeGuard(Fn fn) : fn_(std::move(fn)), active_(true) {}

		ScopeGuard(const ScopeGuard&) = delete;
		ScopeGuard& operator=(const ScopeGuard&) = delete;

		ScopeGuard(ScopeGuard&& other) noexcept
			: fn_(std::move(other.fn_)), active_(other.active_)
		{
			other.dismiss();
		}

		ScopeGuard& operator=(ScopeGuard&&) = delete;

		void dismiss() noexcept { active_ = false; }

		~ScopeGuard()
		{
			if (!active_) return;
			const int uncaught = std::uncaught_exceptions();
			try
			{
				fn_(uncaught);
			}
			catch (...)
			{
				// Destructor must not throw; swallow callback errors.
			}
		}

	private:
		Fn fn_;
		bool active_;
	};

	template <typename Fn>
	ScopeGuard<Fn> make_scope_guard(Fn fn)
	{
		return ScopeGuard<Fn>(std::move(fn));
	}

	// -----------------------------------------------------------------
	// Example: FILE* with custom deleter — RAII over a C API.
	// Prefer iostreams when you can; this shows the unique_ptr pattern.
	// -----------------------------------------------------------------
	struct FileCloser
	{
		void operator()(std::FILE* f) const
		{
			if (f) std::fclose(f);
		}
	};

	using UniqueFile = std::unique_ptr<std::FILE, FileCloser>;

	inline UniqueFile open_unique_file(const char* path, const char* mode)
	{
		std::FILE* raw = nullptr;
#if defined(_MSC_VER)
		if (fopen_s(&raw, path, mode) != 0) raw = nullptr;
#else
		raw = std::fopen(path, mode);
#endif
		return UniqueFile(raw);
	}

} // namespace usage_examples::patterns::behavioral

namespace {

	TEST(RAIIUsageExamples, DestructorReleasesWhenScopeEnds)
	{
		usage_examples::patterns::behavioral::g_live_resources = 0;
		{
			usage_examples::patterns::behavioral::ResourceTicket a(1);
			usage_examples::patterns::behavioral::ResourceTicket b(2);
			EXPECT_EQ(usage_examples::patterns::behavioral::g_live_resources, 2);
		}
		EXPECT_EQ(usage_examples::patterns::behavioral::g_live_resources, 0);
	}

	TEST(RAIIUsageExamples, MoveTransfersOwnershipWithoutDoubleRelease)
	{
		usage_examples::patterns::behavioral::g_live_resources = 0;
		usage_examples::patterns::behavioral::ResourceTicket first(10);
		EXPECT_EQ(usage_examples::patterns::behavioral::g_live_resources, 1);
		usage_examples::patterns::behavioral::ResourceTicket second = std::move(first);
		EXPECT_TRUE(first.empty());
		EXPECT_EQ(second.id(), 10);
		EXPECT_EQ(usage_examples::patterns::behavioral::g_live_resources, 1);
	}

	TEST(RAIIUsageExamples, UniquePtrWithCustomDeleterClosesFile)
	{
		const std::string path = "raii_behavioral_test_temp.txt";
		{
			auto out = usage_examples::patterns::behavioral::open_unique_file(path.c_str(), "wb");
			ASSERT_TRUE(out);
			const char msg[] = "RAII";
			EXPECT_EQ(std::fwrite(msg, 1, sizeof(msg) - 1, out.get()), sizeof(msg) - 1);
		}
		// File closed by UniqueFile destructor before we read.
		std::ifstream in(path, std::ios::binary);
		std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		EXPECT_EQ(content, "RAII");
		std::remove(path.c_str());
	}

	TEST(RAIIUsageExamples, LockGuardReleasesMutexWhenScopeEnds)
	{
		std::mutex m;
		int shared = 0;
		auto worker = [&] {
			std::lock_guard<std::mutex> lock(m);
			++shared;
		};
		worker();
		EXPECT_EQ(shared, 1);
	}

	TEST(RAIIUsageExamples, VectorOwnsMemoryAutomatically)
	{
		{
			std::vector<int> v;
			v.reserve(1000);
			v.push_back(42);
			EXPECT_EQ(v.size(), 1u);
		}
		// No manual delete — storage freed when v is destroyed.
		SUCCEED();
	}

	TEST(RAIIUsageExamples, ScopeGuardRunsAtScopeExit)
	{
		int steps = 0;
		{
			auto guard = usage_examples::patterns::behavioral::make_scope_guard([&](int) { ++steps; });
			(void)guard;
		}
		EXPECT_EQ(steps, 1);
	}

	TEST(RAIIUsageExamples, ScopeGuardSeesUncaughtExceptionCountDuringUnwind)
	{
		int observed_uncaught = -1;
		try
		{
			auto guard = usage_examples::patterns::behavioral::make_scope_guard([&](int uncaught) {
				observed_uncaught = uncaught;
			});
			(void)guard;
			throw std::runtime_error("test unwind");
		}
		catch (const std::runtime_error&)
		{
		}
		EXPECT_GE(observed_uncaught, 1);
	}

} // namespace

/*
 * Further exercises (not compiled here — try them locally):
 *
 * 1. Implement a small `DatabaseConnection` mock: connect in ctor, disconnect
 *    in dtor; write a function with three return paths and verify disconnect
 *    runs once on each path.
 *
 * 2. Combine RAII with `std::optional` or `std::expected` (C++23) to represent
 *    operations that may fail before the resource exists, without leaks.
 *
 * 3. Read about `std::make_unique` / `std::make_shared` — they avoid raw `new`
 *    in user code and give strong exception safety when building object graphs.
 *
 * 4. Compare with `finally` in other languages: in C++, a local ScopeGuard or
 *    a dedicated RAII type is the idiomatic equivalent.
 */
