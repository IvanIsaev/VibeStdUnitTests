/*
 * =============================================================================
 * PIMPL — Pointer to IMPLementation (compilation firewall / structural idiom)
 * =============================================================================
 *
 * Intent
 * ------
 * Move **private members and heavy dependencies** out of the public class
 * interface into a separately defined **implementation type** that clients never
 * see. The public class holds only an **opaque pointer** (usually
 * `std::unique_ptr<Impl>`) to that type.
 *
 * Names: **PIMPL**, **Cheshire Cat**, **opaque pointer**, **compilation firewall**.
 * It is a **structural** pattern: it changes how a type is *composed*, not how
 * algorithms vary.
 *
 * Why use it
 * ----------
 *   • **Faster builds** — headers that include your class no longer transitively
 *     include `<vector>`, `<map>`, third-party SDKs, or internal details. One
 *     `.cpp` recompiles when the private layout changes; dependents only relink.
 *
 *   • **ABI stability** — on some platforms, keeping the public class layout
 *     fixed (one pointer + maybe a vptr) can reduce breakage across library
 *     versions *if* you manage symbol visibility and versioning carefully.
 *
 *   • **Encapsulation** — private state is literally invisible; users cannot
 *     depend on accidental details.
 *
 * Classic layout (header vs source)
 * ---------------------------------
 *
 *   // Widget.h — minimal includes, forward declaration only
 *   #include <memory>
 *   class Widget {
 *     struct Impl;
 *     std::unique_ptr<Impl> impl_;
 *    public:
 *     Widget();
 *     ~Widget();
 *     Widget(Widget&&) noexcept;
 *     Widget& operator=(Widget&&) noexcept;
 *     Widget(const Widget&) = delete;
 *     Widget& operator=(const Widget&) = delete;
 *     void do_work();
 *   };
 *
 *   // Widget.cpp
 *   #include "Widget.h"
 *   #include <vector>
 *   #include "HeavyThirdParty.h"
 *   struct Widget::Impl {
 *     std::vector<int> data;
 *     HeavyClient sdk;
 *   };
 *   Widget::Widget() : impl_(std::make_unique<Impl>()) {}
 *   Widget::~Widget() = default;
 *   ...
 *
 * std::unique_ptr and incomplete types (critical in C++)
 * ------------------------------------------------------
 * `std::unique_ptr<Impl>` **deletes** `Impl` in its destructor using a
 * **deleter** instantiated where `Impl` must be **complete**. Therefore:
 *
 *   • The **destructor** of the outer class must be **defined** in a `.cpp`
 *     where `struct Widget::Impl { ... };` is already visible — not `= default`
 *     inline in the header unless `Impl` is complete there (defeating PIMPL).
 *
 *   • The same applies to **move constructor** and **move assignment** if they
 *     are defaulted in the header — they must be **out-of-line defaulted** in
 *     the `.cpp`, or explicitly implemented there.
 *
 *   • **Copy** operations usually require deep-copy logic on `Impl` and belong
 *     in the `.cpp` as well (or are deleted if the type should be move-only).
 *
 * Alternatives and combinations
 * -----------------------------
 *   • **Interface + factory** — pure abstract base + hidden derived in `.cpp`;
 *     no member `unique_ptr`, but another level of indirection.
 *
 *   • **std::shared_ptr<Impl>** — if many handles share one implementation; watch
 *     same completeness rules for custom deleters if you inline odd places.
 *
 *   • **Fast PIMPL / small buffer** — embed a few bytes inside the public object
 *     and only heap-allocate when the implementation grows; reduces allocations
 *     but complicates layout (boost::container::small_vector-style thinking).
 *
 * Trade-offs
 * ----------
 *   • **Extra indirection** — one pointer hop; usually negligible vs I/O or heavy
 *     work, but not ideal for the tiniest hot structs.
 *
 *   • **Boilerplate** — forwarding functions, explicit out-of-line special members.
 *
 *   • **Debugging** — one more layer when stepping; names like `impl_->` help.
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   // Forwarding a method in the public class:
 *   void Widget::add(int x) { impl_->data.push_back(x); }
 *
 *   // Optional: const-correct access
 *   int Widget::size() const { return static_cast<int>(impl_->data.size()); }
 *
 * Pitfalls
 * --------
 *   • **inline defaulted destructor** in header with incomplete `Impl` → often
 *     **ill-formed** or confusing errors about `delete` on incomplete type.
 *
 *   • **Forgotten move ops** — type becomes immovable or accidentally copies
 *     `unique_ptr` incorrectly.
 *
 *   • **Exporting inline templates** on the public class that touch `Impl` in the
 *     header — pulls implementation details back into headers.
 *
 * Testing
 * -------
 * Test only the **public contract**. The point of PIMPL is that tests (and
 * clients) do not include private headers; use link-time or `.cpp`-local tests.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace usage_examples::patterns::structural {

	// -----------------------------------------------------------------
	// Public surface — in a real project this block would live in a .h with
	// only <memory> (and maybe string forward decl). Implementation follows
	// after Impl is defined in this same .cpp (simulating Widget.cpp).
	// -----------------------------------------------------------------
	class Widget
	{
	public:
		Widget();
		~Widget();

		Widget(Widget&&) noexcept;
		Widget& operator=(Widget&&) noexcept;

		Widget(const Widget&) = delete;
		Widget& operator=(const Widget&) = delete;

		void push_back(int value);
		[[nodiscard]] int sum() const;
		[[nodiscard]] std::string label() const;
		void set_label(std::string value);

	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;
	};

	// Heavy / private state — would live in Widget.cpp together with method defs.
	struct Widget::Impl
	{
		std::vector<int> numbers;
		std::string label{ "pimpl-demo" };
	};

	inline Widget::Widget() : impl_(std::make_unique<Impl>()) {}

	inline Widget::~Widget() = default;

	inline Widget::Widget(Widget&&) noexcept = default;

	inline Widget& Widget::operator=(Widget&&) noexcept = default;

	inline void Widget::push_back(int value)
	{
		impl_->numbers.push_back(value);
	}

	inline int Widget::sum() const
	{
		int total = 0;
		for (int n : impl_->numbers) total += n;
		return total;
	}

	inline std::string Widget::label() const { return impl_->label; }

	inline void Widget::set_label(std::string value)
	{
		impl_->label = std::move(value);
	}

	// -----------------------------------------------------------------
	// Second miniature type — move-only handle with hidden dependency string.
	// -----------------------------------------------------------------
	class CounterHandle
	{
	public:
		CounterHandle();
		~CounterHandle();

		CounterHandle(CounterHandle&&) noexcept;
		CounterHandle& operator=(CounterHandle&&) noexcept;

		CounterHandle(const CounterHandle&) = delete;
		CounterHandle& operator=(const CounterHandle&) = delete;

		void increment();
		[[nodiscard]] int value() const;

	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;
	};

	struct CounterHandle::Impl
	{
		int count{ 0 };
		std::string audit_tag{ "counter" };
	};

	inline CounterHandle::CounterHandle() : impl_(std::make_unique<Impl>()) {}

	inline CounterHandle::~CounterHandle() = default;

	inline CounterHandle::CounterHandle(CounterHandle&&) noexcept = default;

	inline CounterHandle& CounterHandle::operator=(CounterHandle&&) noexcept = default;

	inline void CounterHandle::increment()
	{
		++impl_->count;
	}

	inline int CounterHandle::value() const { return impl_->count; }

} // namespace usage_examples::patterns::structural

namespace {

	using usage_examples::patterns::structural::CounterHandle;
	using usage_examples::patterns::structural::Widget;

	TEST(PIMPLUsageExamples, WidgetHidesVectorBehindPublicApi)
	{
		Widget w;
		w.push_back(3);
		w.push_back(10);
		EXPECT_EQ(w.sum(), 13);
		w.set_label("test");
		EXPECT_EQ(w.label(), "test");
	}

	TEST(PIMPLUsageExamples, WidgetIsMovable)
	{
		Widget a;
		a.push_back(7);
		Widget b = std::move(a);
		EXPECT_EQ(b.sum(), 7);
		Widget c;
		c = std::move(b);
		EXPECT_EQ(c.sum(), 7);
	}

	TEST(PIMPLUsageExamples, CounterHandleDemonstratesSameIdiom)
	{
		CounterHandle h;
		h.increment();
		h.increment();
		EXPECT_EQ(h.value(), 2);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Split this file into `Widget.h` / `Widget.cpp` and include the header from
 *    another `.cpp` that only calls `Widget::sum()` — confirm the consumer TU
 *    does not need `<vector>`.
 * 2. Add `Widget(const Widget&)` deep copy in `Widget.cpp` by allocating a new
 *    `Impl` and copying `impl_->numbers` and `label`.
 * 3. Compare with an **interface class** + `std::unique_ptr<IWidget>` factory
 *    returning a hidden derived type — similar compile firewall, different cost.
 */
