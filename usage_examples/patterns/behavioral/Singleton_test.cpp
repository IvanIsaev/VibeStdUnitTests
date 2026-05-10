/*
 * =============================================================================
 * Singleton (Gang of Four — Creational; placed here with other usage examples)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Ensure a class has *only one instance* and provide a *global point of access*
 * to it.
 *
 * Typical structure
 * -----------------
 *   • Private (or protected) constructors — block `new T()` from clients.
 *   • Deleted copy/move — no second instance by copying.
 *   • Static accessor — `T& instance()` (or `get()`) returns the sole object.
 *
 * In modern C++, the accessor almost always uses a *function-local static* object
 * (the "Meyers singleton"): initialization is lazy, thread-safe (since C++11),
 * and the compiler handles destruction at program exit (with the usual caveats
 * about destruction order across translation units).
 *
 * Why Singleton became controversial
 * -----------------------------------
 *   • **Hidden dependencies** — any code can reach `Thing::instance()`; call
 *     graphs become hard to see compared with constructor injection.
 *
 *   • **Testing** — global state persists across tests; parallel tests may race;
 *     replacing the implementation for a mock often requires indirection or
 *     preprocessor seams.
 *
 *   • **Lifecycle** — "exactly one for entire program" is often the wrong scope;
 *     you may need one per thread, one per request, or one per subsystem.
 *
 *   • **Configuration** — hard to supply different policies per deployment
 *     without mutating the singleton after creation (another global).
 *
 * Preferred alternatives (often)
 * ------------------------------
 *   • Pass interfaces into constructors (**dependency injection**).
 *   • Use a small **context** or **service** object owned by `main` / composition root.
 *   • **Monostate** — all instances share static data; still global state, but
 *     looser coupling to "one object identity."
 *
 * When Singleton can still be reasonable
 * ---------------------------------------
 *   • Thin wrappers around **true OS singletons** (e.g. one process-wide logging
 *     sink where the platform already enforces uniqueness).
 *   • **Expensive read-only** caches loaded once (still consider DI for tests).
 *   • Legacy APIs that require a global registration point — isolate behind an
 *     interface and keep the singleton at the edge of the system.
 *
 * C++ implementation: Meyers singleton (recommended baseline)
 * ------------------------------------------------------------
 *
 *   class Config {
 *    public:
 *     static Config& instance() {
 *       static Config c;  // thread-safe lazy init (C++11 onward)
 *       return c;
 *     }
 *     Config(const Config&) = delete;
 *     Config& operator=(const Config&) = delete;
 *    private:
 *     Config() = default;
 *   };
 *
 * Avoid the classic **double-checked locking** idiom on raw pointers in C++;
 * pre-C++11 it was easy to get wrong without proper barriers. Prefer Meyers or
 * `std::call_once`.
 *
 * std::call_once (when you need non-default-constructible or two-phase init)
 * --------------------------------------------------------------------------
 *
 *   class Heavy {
 *    public:
 *     static Heavy& instance() {
 *       std::call_once(flag_, init);
 *       return *ptr_;
 *     }
 *    private:
 *     static void init() { ptr_ = std::make_unique<Heavy>(load_from_disk()); }
 *     static std::once_flag flag_;
 *     static std::unique_ptr<Heavy> ptr_;
 *   };
 *
 * Meyers vs call_once
 * -------------------
 *   • Meyers: minimal code; object destroys in reverse order of completion of
 *     dynamic initialization within the same TU; simple types rule.
 *   • call_once: explicit control when construction needs parameters or might
 *     fail (consider `std::expected` / exceptions and leaving `ptr_` null).
 *
 * Thread-safety note
 * ------------------
 * Meyers `static` local initialization is guarded by the implementation.
 * If the singleton's *methods* mutate shared state, you must still synchronize
 * (mutex, atomics) — the pattern only helps with *one-time construction*.
 *
 * Destruction and atexit
 * ----------------------
 * Singletons are destroyed as part of static teardown. Do not access them from
 * other static destructors unless you control ordering (e.g. `atexit` order,
 * or avoid the dependency entirely).
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   // C++17 inline variable — "one bool per program" without a class:
 *   inline std::atomic<bool> g_feature_on{false};
 *
 *   // Per-thread "singleton" — one instance *per thread*:
 *   thread_local int tls_scratch = 0;
 *
 *   // Testing seam: interface + default singleton accessor
 *   struct Clock { virtual ~Clock() = default; virtual int now() = 0; };
 *   Clock& system_clock(); // might return Meyers or a test double wired in main
 *
 * Pitfalls checklist
 * ------------------
 *   • Public non-deleted copy/move → multiple live instances.
 *   • Singleton holding pointers to objects destroyed earlier → UAF during exit.
 *   • Throwing constructor in Meyers — first call propagates; retry semantics
 *     are not "try again until success" unless you code that carefully.
 *   • Subclassing Meyers base with CRTP — possible, but easy to confuse types;
 *     document the intended hierarchy.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>

namespace usage_examples::patterns::behavioral {

	// -----------------------------------------------------------------
	// Meyers Singleton — lazy, thread-safe single instance (C++11+).
	// -----------------------------------------------------------------
	class TicketSerialMeyers
	{
	public:
		[[nodiscard]] static TicketSerialMeyers& instance()
		{
			static TicketSerialMeyers single;
			return single;
		}

		TicketSerialMeyers(const TicketSerialMeyers&) = delete;
		TicketSerialMeyers& operator=(const TicketSerialMeyers&) = delete;
		TicketSerialMeyers(TicketSerialMeyers&&) = delete;
		TicketSerialMeyers& operator=(TicketSerialMeyers&&) = delete;

		~TicketSerialMeyers() = default;

		[[nodiscard]] int next_ticket() { return ++counter_; }

		[[nodiscard]] int last_value() const { return counter_; }

	private:
		TicketSerialMeyers() = default;

		std::atomic<int> counter_{ 0 };
	};

	// -----------------------------------------------------------------
	// call_once + unique_ptr — useful when construction is non-trivial or
	// you want a single heap block (still one logical instance).
	// -----------------------------------------------------------------
	class SessionTokenCacheOnce
	{
	public:
		[[nodiscard]] static SessionTokenCacheOnce& instance()
		{
			std::call_once(init_flag_, [] { storage_ = std::make_unique<SessionTokenCacheOnce>(); });
			return *storage_;
		}

		SessionTokenCacheOnce(const SessionTokenCacheOnce&) = delete;
		SessionTokenCacheOnce& operator=(const SessionTokenCacheOnce&) = delete;

		// Public constructor so std::make_unique (and the call_once lambda) can
		// create the instance; callers should still use instance() for the true
		// singleton entry point.
		SessionTokenCacheOnce() = default;

		void store(std::size_t key, int value)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			// Tiny fixed demo table — not a real cache.
			if (key >= 4) return;
			slots_[key] = value;
		}

		[[nodiscard]] int load(std::size_t key) const
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (key >= 4) return -1;
			return slots_[key];
		}

		~SessionTokenCacheOnce() = default;

	private:
		mutable std::mutex mutex_{};
		int slots_[4]{};

		static std::once_flag init_flag_;
		static std::unique_ptr<SessionTokenCacheOnce> storage_;
	};

	inline std::once_flag SessionTokenCacheOnce::init_flag_;
	inline std::unique_ptr<SessionTokenCacheOnce> SessionTokenCacheOnce::storage_;

	// -----------------------------------------------------------------
	// "Monostate" sketch — many objects, shared static data (not GoF Singleton,
	// but often mentioned alongside it). All instances see the same counter.
	// -----------------------------------------------------------------
	class SharedHitCounter
	{
	public:
		void record_hit() { ++hits_; }
		[[nodiscard]] static int total_hits() { return hits_; }

	private:
		static std::atomic<int> hits_;
	};

	inline std::atomic<int> SharedHitCounter::hits_{ 0 };

} // namespace usage_examples::patterns::behavioral

namespace {

	using usage_examples::patterns::behavioral::SessionTokenCacheOnce;
	using usage_examples::patterns::behavioral::SharedHitCounter;
	using usage_examples::patterns::behavioral::TicketSerialMeyers;

	TEST(SingletonUsageExamples, MeyersInstanceHasStableAddress)
	{
		EXPECT_EQ(&TicketSerialMeyers::instance(), &TicketSerialMeyers::instance());
	}

	TEST(SingletonUsageExamples, MeyersIssuesMonotonicTicketNumbers)
	{
		auto& gen = TicketSerialMeyers::instance();
		const int a = gen.next_ticket();
		const int b = gen.next_ticket();
		EXPECT_EQ(b, a + 1);
	}

	TEST(SingletonUsageExamples, CallOnceCacheIsSingleInstance)
	{
		EXPECT_EQ(&SessionTokenCacheOnce::instance(), &SessionTokenCacheOnce::instance());
		SessionTokenCacheOnce::instance().store(1, 42);
		EXPECT_EQ(SessionTokenCacheOnce::instance().load(1), 42);
	}

	TEST(SingletonUsageExamples, MonostateSharesStateAcrossValueObjects)
	{
		SharedHitCounter a;
		SharedHitCounter b;
		const int before = SharedHitCounter::total_hits();
		a.record_hit();
		b.record_hit();
		EXPECT_EQ(SharedHitCounter::total_hits(), before + 2);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Wrap a Meyers singleton behind an interface and inject a test double from
 *    `main` in unit tests (replace the accessor or use a small factory).
 * 2. Read about the "static initialization order fiasco" for non-local statics
 *    and why function-local statics avoid the worst of it.
 * 3. Compare with `std::latch` / one-shot initialization in concurrent modules
 *    when teardown order must be coordinated explicitly.
 */
