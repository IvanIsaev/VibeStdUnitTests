/*
 * =============================================================================
 * CRTP — Curiously Recurring Template Pattern (compile-time polymorphism)
 * =============================================================================
 *
 * What it is
 * ----------
 * A class `B` is a **class template** parameterized by its **derived** class `D`,
 * and `D` inherits from `B<D>`. The base uses `static_cast<D*>(this)` to call
 * methods on the *actual* derived type **without** `virtual` functions.
 *
 *   template <class Derived>
 *   struct Base {
 *     void interface() {
 *       static_cast<Derived*>(this)->implementation();
 *     }
 *   };
 *   struct Derived : Base<Derived> {
 *     void implementation() { ... }  // body omitted
 *   };
 *
 * This is **static** (compile-time) polymorphism: the callee is resolved during
 * template instantiation — **no vtable**, **no runtime dispatch** on that path.
 * It is a **structural** idiom: you express “is-a” through inheritance, but the
 * relationship is encoded in templates rather than a single runtime base type.
 *
 * Why use it
 * ----------
 *   • **Hot paths** where virtual calls are undesirable (embedded, HPC, games)
 *     — measure first; many compilers devirtualize well too.
 *
 *   • **Mixins** — compose small CRTP bases (`Serializable<Derived>`, `Hashable<…>`)
 *     that inject members or friend operators.
 *
 *   • **Policy-based design** (Alexandrescu) — a host class is parameterized by
 *     policy types that customize behavior at compile time.
 *
 *   • **Avoiding a common runtime base** — no single `Shape*` if every concrete
 *     shape is a different template instance; use `std::variant` or templates
 *     at the call site when you need a collection.
 *
 * CRTP vs virtual polymorphism
 * ----------------------------
 *   • **Virtual:** one stable base type (`Shape&`), runtime dispatch, vptr cost,
 *     easy heterogeneous containers of `unique_ptr<Shape>`.
 *
 *   • **CRTP:** each `Circle` is unrelated to `Rectangle` at the type level;
 *     generic code uses `template<class D> void use(Proto<D>&)` or concepts.
 *
 * Barton–Nackman trick (operators via CRTP)
 * -----------------------------------------
 * Implement symmetric operators as **friends** in a CRTP base so `Derived` gets
 * `operator==` without repeating boilerplate:
 *
 *   template <class Derived>
 *   struct EqualityComparable { ... };
 *
 * Mixins and linear inheritance
 * -----------------------------
 * CRTP bases chain cleanly when each adds a small capability:
 *   `struct Widget : Logging<Widget>, Metrics<Widget> { ... };`
 * Watch for **name hiding** and **method ambiguity**; sometimes a single mixin
 * aggregates policies instead.
 *
 * Pitfalls
 * --------
 *   • **Wrong template argument** — `struct Evil : Base<Good> {}` compiles but
 *     `static_cast` is **undefined behavior** if `implementation()` is invoked.
 *     Prefer `static_assert(std::is_base_of_v<Base<Derived>, Derived>)` in the
 *     base (careful with incomplete `Derived` in some contexts) or private
 *     constructors + friend `Derived` patterns for controlled use.
 *
 *   • **Binary interfaces** — CRTP types are not a stable **ABI** across library
 *     boundaries the way a C-style or `virtual` interface can be; keep CRTP in
 *     headers inside one module.
 *
 *   • **Compile-time cost** — many instantiations → larger binaries; virtual
 *     can deduplicate one `draw()` body.
 *
 *   • **Diagnostics** — errors deep in the base template can be verbose; small
 *     `concept` constraints (C++20) on `Derived` improve messages.
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   // Policy: how to store (stack vs heap) — compile-time choice.
 *   template <class Derived, class StoragePolicy>
 *   struct Host : StoragePolicy { ... };
 *
 *   // CRTP + std::derived_from (C++20) in a function template:
 *   template <std::derived_from<ReaderBase<Derived>> Derived>
 *   void load(Derived& r) { r.read_chunk(); }
 *
 * Testing
 * -------
 * Exercise public methods of concrete types; CRTP bases are implementation
 * details tested indirectly unless you factor them into reusable units.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace usage_examples::patterns::structural {

	// -----------------------------------------------------------------
	// Example 1 — “static strategy”: base forwards to Derived::step_impl().
	// -----------------------------------------------------------------
	template <class Derived>
	struct PipelineStep
	{
		void run()
		{
			static_cast<Derived*>(this)->step_impl();
		}
	};

	struct UppercaseStep final : PipelineStep<UppercaseStep>
	{
		std::string buffer;

		explicit UppercaseStep(std::string s) : buffer(std::move(s)) {}

		void step_impl()
		{
			for (char& c : buffer)
				if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
		}

		[[nodiscard]] const std::string& text() const { return buffer; }
	};

	// -----------------------------------------------------------------
	// Example 2 — CRTP mixin for equality (Barton–Nackman style).
	// Derived supplies equal_to(const Derived&) const.
	// -----------------------------------------------------------------
	template <class Derived>
	struct EqualityComparableCRTP
	{
		[[nodiscard]] friend bool operator==(const Derived& a, const Derived& b) { return a.equal_to(b); }

		[[nodiscard]] friend bool operator!=(const Derived& a, const Derived& b) { return !(a == b); }
	};

	struct Point final : EqualityComparableCRTP<Point>
	{
		int x{};
		int y{};

		Point() = default;
		Point(int ax, int ay) : x(ax), y(ay) {}

		[[nodiscard]] bool equal_to(const Point& other) const { return x == other.x && y == other.y; }
	};

	// -----------------------------------------------------------------
	// Example 3 — Counter mixin: increment hooks for metrics/logging.
	// -----------------------------------------------------------------
	template <class Derived>
	struct CountingMixin
	{
		void touch()
		{
			++count_;
			static_cast<Derived*>(this)->on_touch(count_);
		}

		[[nodiscard]] int touch_count() const { return count_; }

	private:
		int count_{ 0 };
	};

	struct Resource final : CountingMixin<Resource>
	{
		int last_reported{ 0 };

		void on_touch(int n) { last_reported = n; }
	};

	// -----------------------------------------------------------------
	// Example 4 — Clone via CRTP + abstract interface for heterogeneous storage.
	// Each derived clone() returns unique_ptr<AbstractDoc>; implementation uses
	// covariant-style make_unique of concrete type.
	// -----------------------------------------------------------------
	struct Document
	{
		virtual ~Document() = default;
		[[nodiscard]] virtual std::unique_ptr<Document> clone() const = 0;
		[[nodiscard]] virtual std::string kind() const = 0;
	};

	template <class Derived>
	struct DocumentCRTP : Document
	{
		[[nodiscard]] std::unique_ptr<Document> clone() const override
		{
			return std::make_unique<Derived>(static_cast<const Derived&>(*this));
		}
	};

	struct NoteDoc final : DocumentCRTP<NoteDoc>
	{
		std::string body;

		explicit NoteDoc(std::string b) : body(std::move(b)) {}

		[[nodiscard]] std::string kind() const override { return "note"; }
	};

	struct MemoDoc final : DocumentCRTP<MemoDoc>
	{
		int priority{};

		explicit MemoDoc(int p) : priority(p) {}

		[[nodiscard]] std::string kind() const override { return "memo"; }
	};

} // namespace usage_examples::patterns::structural

namespace {

	using usage_examples::patterns::structural::MemoDoc;
	using usage_examples::patterns::structural::NoteDoc;
	using usage_examples::patterns::structural::Point;
	using usage_examples::patterns::structural::Resource;
	using usage_examples::patterns::structural::UppercaseStep;

	TEST(CRTPUsageExamples, PipelineStepDispatchesToDerivedWithoutVirtual)
	{
		UppercaseStep step("abC");
		step.run();
		EXPECT_EQ(step.text(), "ABC");
	}

	TEST(CRTPUsageExamples, EqualityComparableUsesDerivedEqualTo)
	{
		const Point p{ 1, 2 };
		const Point q{ 1, 2 };
		const Point r{ 1, 3 };
		EXPECT_TRUE(p == q);
		EXPECT_FALSE(p == r);
		EXPECT_TRUE(p != r);
	}

	TEST(CRTPUsageExamples, CountingMixinForwardsToOnTouch)
	{
		Resource res;
		res.touch();
		res.touch();
		EXPECT_EQ(res.touch_count(), 2);
		EXPECT_EQ(res.last_reported, 2);
	}

	TEST(CRTPUsageExamples, DocumentCRTPClonePreservesDynamicKind)
	{
		const NoteDoc original("hello");
		const auto copy = original.clone();
		ASSERT_TRUE(copy);
		EXPECT_EQ(copy->kind(), "note");
		const auto* note = dynamic_cast<const NoteDoc*>(copy.get());
		ASSERT_NE(note, nullptr);
		EXPECT_EQ(note->body, "hello");

		const MemoDoc memo(5);
		const auto copy2 = memo.clone();
		ASSERT_TRUE(copy2);
		EXPECT_EQ(copy2->kind(), "memo");
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add a C++20 `concept` that requires `step_impl()` on types used with
 *    `PipelineStep`, and use it to constrain free function templates.
 * 2. Try a small **policy-based** logger: `template<class Derived, class Sink>`
 *    where `Sink` is a class with `static void write(std::string_view)`.
 * 3. Compare codegen: disassemble or Compiler Explorer a CRTP `run()` vs a
 *    `virtual void run()` in a tight loop for your target and optimization level.
 */
