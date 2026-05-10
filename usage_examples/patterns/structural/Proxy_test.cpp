/*
 * =============================================================================
 * Proxy (Gang of Four — Structural)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Provide a **surrogate** or **placeholder** for another object to **control
 * access** to it.
 *
 * Clients interact with the Proxy through the **same** abstract interface as the
 * **RealSubject**; the Proxy forwards most work but can **delay**, **guard**,
 * **log**, **cache**, or **remote-delegate** as needed.
 *
 * Common proxy flavors
 * --------------------
 *   • **Virtual proxy** — **lazy creation** or expensive loading (decode image,
 *     open DB) on first real use.
 *
 *   • **Protection proxy** — **authorization**, quotas, validation before the
 *     real object runs.
 *
 *   • **Remote proxy** — stand-in for an object in **another address space**
 *     (RPC, REST client stub); marshals calls and returns futures or values.
 *
 *   • **Smart reference** — manage **lifetime** or **sharing** (compare
 *     `std::shared_ptr` control block — related idea, not identical to GoF).
 *
 * Typical structure
 * -----------------
 *   • **Subject** — interface shared by RealSubject and Proxy.
 *   • **RealSubject** — heavy or sensitive implementation.
 *   • **Proxy** — holds a pointer or lazy `unique_ptr` to RealSubject; implements
 *     Subject by forwarding (sometimes after pre/post work).
 *   • **Client** — depends on Subject; stays unaware whether it holds a Proxy.
 *
 * Proxy vs Decorator (intent matters)
 * -----------------------------------
 *   • **Decorator** usually **stacks** optional **features** (border, scroll,
 *     metrics) and often nests several layers.
 *
 *   • **Proxy** usually **controls access** to **one** real object: load on
 *     demand, check permissions, hide network latency. Implementations can look
 *     alike — **name and documented purpose** keep teams aligned.
 *
 * Proxy vs Adapter
 * ----------------
 *   • **Adapter** makes **different** interfaces work together.
 *   • **Proxy** keeps the **same** interface while interposing behavior.
 *
 * C++ implementation notes
 * ------------------------
 *   • **Lazy virtual proxy:** `mutable std::unique_ptr<RealSubject>` created in
 *     a private `ensure_loaded() const` helper when `const` methods trigger load.
 *
 *   • **Thread safety:** if the Proxy may be shared across threads, protect
 *     lazy init with `std::mutex` or `std::call_once` (not shown below).
 *
 *   • **Copy/move:** decide whether copying the Proxy duplicates the handle,
 *     shares the RealSubject, or is deleted — document clearly.
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   // Remote proxy sketch (pseudocode):
 *   class UserServiceProxy : IUserService {
 *    public:
 *     User get_user(Id id) override { return rpc_.call("get_user", id); }
 *    private:
 *     GrpcChannel rpc_;
 *   };
 *
 *   // Caching proxy:
 *   int fib(int n) {
 *     if (auto it = cache_.find(n); it != cache_.end()) return it->second;
 *     int v = real_.fib(n);
 *     cache_[n] = v;
 *     return v;
 *   }
 *
 * Pitfalls
 * --------
 *   • **Double initialization** — lazy proxies must create the real object
 *     exactly once (or use a well-defined per-thread policy).
 *
 *   • **Exception safety** — if construction of RealSubject throws, the Proxy
 *     should remain usable or surface a clear error on retry.
 *
 *   • **Identity** — `proxy.get() != &real` breaks naive pointer equality; avoid
 *     exposing raw addresses if clients compare them.
 *
 * Testing
 * -------
 *   • Count **how often** the real resource is constructed or how many remote
 *     calls fire — proxies exist partly to bound that cost.
 *   • **Protection:** assert denied paths never mutate the real subject.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

namespace usage_examples::patterns::structural {

	// Test hook: how many expensive bitmaps were actually constructed.
	inline int g_real_bitmap_load_count = 0;

	// -----------------------------------------------------------------
	// Virtual proxy — defer decoding until a pixel query happens.
	// -----------------------------------------------------------------
	struct IBitmap
	{
		virtual ~IBitmap() = default;
		[[nodiscard]] virtual int width() const = 0;
		[[nodiscard]] virtual int height() const = 0;
	};

	class RealBitmap final : public IBitmap
	{
	public:
		explicit RealBitmap(std::string /*path_from_disk*/)
		{
			++g_real_bitmap_load_count;
			width_ = 1920;
			height_ = 1080;
		}

		[[nodiscard]] int width() const override { return width_; }
		[[nodiscard]] int height() const override { return height_; }

	private:
		int width_{};
		int height_{};
	};

	class LazyBitmapProxy final : public IBitmap
	{
	public:
		explicit LazyBitmapProxy(std::string path) : path_(std::move(path)) {}

		LazyBitmapProxy(const LazyBitmapProxy&) = delete;
		LazyBitmapProxy& operator=(const LazyBitmapProxy&) = delete;
		LazyBitmapProxy(LazyBitmapProxy&&) noexcept = default;
		LazyBitmapProxy& operator=(LazyBitmapProxy&&) noexcept = default;

		[[nodiscard]] int width() const override
		{
			ensure_loaded();
			return real_->width();
		}

		[[nodiscard]] int height() const override
		{
			ensure_loaded();
			return real_->height();
		}

	private:
		void ensure_loaded() const
		{
			if (!real_) real_ = std::make_unique<RealBitmap>(path_);
		}

		mutable std::unique_ptr<RealBitmap> real_;
		std::string path_;
	};

	// -----------------------------------------------------------------
	// Protection proxy — gate read access (authorization / policy).
	// -----------------------------------------------------------------
	struct ISecretDocument
	{
		virtual ~ISecretDocument() = default;
		[[nodiscard]] virtual std::string body() const = 0;
	};

	class VaultDocument final : public ISecretDocument
	{
	public:
		explicit VaultDocument(std::string text) : text_(std::move(text)) {}

		[[nodiscard]] std::string body() const override { return text_; }

	private:
		std::string text_;
	};

	class AuthorizedReadProxy final : public ISecretDocument
	{
	public:
		AuthorizedReadProxy(const VaultDocument& real, bool reader_is_trusted) : real_(real), trusted_(reader_is_trusted)
		{}

		[[nodiscard]] std::string body() const override
		{
			if (!trusted_) return std::string("[redacted]");
			return real_.body();
		}

	private:
		const VaultDocument& real_;
		bool trusted_;
	};

	// -----------------------------------------------------------------
	// Logging proxy — counts forwarded calls (lightweight “smart reference”).
	// -----------------------------------------------------------------
	struct ICounterService
	{
		virtual ~ICounterService() = default;
		virtual void bump() = 0;
		[[nodiscard]] virtual int value() const = 0;
	};

	class SimpleCounter final : public ICounterService
	{
	public:
		void bump() override { ++n_; }
		[[nodiscard]] int value() const override { return n_; }

	private:
		int n_{ 0 };
	};

	class CountingCallProxy final : public ICounterService
	{
	public:
		explicit CountingCallProxy(SimpleCounter& inner) : inner_(inner) {}

		void bump() override
		{
			++bump_calls_;
			inner_.bump();
		}

		[[nodiscard]] int value() const override { return inner_.value(); }

		[[nodiscard]] int bump_calls_recorded() const { return bump_calls_; }

	private:
		SimpleCounter& inner_;
		int bump_calls_{ 0 };
	};

} // namespace usage_examples::patterns::structural

namespace {

	using usage_examples::patterns::structural::AuthorizedReadProxy;
	using usage_examples::patterns::structural::CountingCallProxy;
	using usage_examples::patterns::structural::g_real_bitmap_load_count;
	using usage_examples::patterns::structural::LazyBitmapProxy;
	using usage_examples::patterns::structural::SimpleCounter;
	using usage_examples::patterns::structural::VaultDocument;

	TEST(ProxyUsageExamples, LazyBitmapDefersRealLoadUntilQueried)
	{
		g_real_bitmap_load_count = 0;
		const LazyBitmapProxy proxy("assets/hero.png");
		EXPECT_EQ(g_real_bitmap_load_count, 0);

		EXPECT_EQ(proxy.width(), 1920);
		EXPECT_EQ(g_real_bitmap_load_count, 1);

		EXPECT_EQ(proxy.height(), 1080);
		EXPECT_EQ(g_real_bitmap_load_count, 1);
	}

	TEST(ProxyUsageExamples, ProtectionProxyRedactsWhenUntrusted)
	{
		const VaultDocument vault("launch-codes");
		const AuthorizedReadProxy guest(vault, false);
		const AuthorizedReadProxy admin(vault, true);

		EXPECT_EQ(guest.body(), "[redacted]");
		EXPECT_EQ(admin.body(), "launch-codes");
	}

	TEST(ProxyUsageExamples, CountingProxyObservesForwardedCalls)
	{
		SimpleCounter real;
		CountingCallProxy proxy(real);
		proxy.bump();
		proxy.bump();
		EXPECT_EQ(real.value(), 2);
		EXPECT_EQ(proxy.bump_calls_recorded(), 2);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add `std::mutex` + `std::once_flag` around `ensure_loaded()` for a shared
 *    lazy proxy used from multiple threads.
 * 2. Implement a **remote** facade that counts serialized RPC bytes while matching
 *    the same `IService` interface as an in-process `RealService`.
 * 3. Compare with **Decorator** by naming: would “add caching” be a new Decorator
 *    layer or a Proxy replacing the real subject? Write the team guideline.
 */
