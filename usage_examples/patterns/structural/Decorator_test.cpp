/*
 * =============================================================================
 * Decorator (Gang of Four — Structural)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Attach **additional responsibilities** to an object **dynamically** and
 * transparently. Decorators provide a flexible alternative to **subclassing**
 * for extending behavior.
 *
 * Instead of `BoldItalicUnderlinedLabel` combinatorial subclasses, you compose
 * small decorator objects around a **component** at runtime: border, scroll,
 * metrics, caching, compression, and so on — in any order you allow.
 *
 * Typical structure
 * -----------------
 *   • **Component** — common interface (`render`, `cost`, `read`).
 *   • **ConcreteComponent** — the core object being wrapped.
 *   • **Decorator** — maintains a reference (or `unique_ptr`) to a Component and
 *     conforms to the same interface.
 *   • **ConcreteDecorator** — adds behavior before or after forwarding to the
 *     inner component (or replaces the operation entirely).
 *
 * Why use it
 * ----------
 *   • **Open/Closed** — add features by new decorator classes, not by editing
 *     every variant of the core type.
 *
 *   • **Runtime composition** — choose features per instance (UI themes, I/O
 *     pipelines) without a compile-time explosion of class names.
 *
 *   • **Recursive structure** — decorators can wrap decorators; order defines
 *     how layers stack (document ordering rules clearly).
 *
 * Decorator vs subclassing
 * ------------------------
 *   • Subclassing fixes behavior at compile time; each combination may need a
 *     new class (`BorderedScrollingFrame` …).
 *
 *   • Decorators **nest** at runtime: `Border(Scroll(Frame(x)))` from a small
 *     set of decorator types.
 *
 * Decorator vs related patterns
 * -----------------------------
 *   • **Adapter** changes an interface to match what a client expects; the
 *     Decorator keeps the **same** interface and **adds** responsibilities.
 *
 *   • **Proxy** often controls access (lazy load, security, logging at the
 *     boundary); **Decorator** usually stacks **multiple** optional features.
 *     In practice the implementations can look similar — intent and naming guide
 *     maintainers.
 *
 *   • **Composite** treats part-whole hierarchies (trees); Decorator is usually
 *     a single chain of one inner component (though you can combine ideas).
 *
 * C++ implementation notes
 * ------------------------
 *   • Own the inner component with **`std::unique_ptr<Component>`** when the
 *     decorator creates or receives ownership; use **`Component&`** when the
 *     caller owns the core and the decorator is short-lived.
 *
 *   • **Forward** every `Component` method you do not override, or make a thin
 *     abstract `Decorator` base that centralizes the pointer and helper calls.
 *
 *   • **Const-correctness** — if `render() const` is part of the contract,
 *     decorators must be `const`-friendly (no hidden mutation in `const` APIs).
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   // std::iostream stack is decorator-like: buffering, locale facets wrap
 *   // streambuf behavior — not pure GoF, but the same compositional idea.
 *
 *   std::unique_ptr<IText> t = std::make_unique<Plain>("hi");
 *   t = std::make_unique<Bold>(std::move(t));
 *   t = std::make_unique<Italic>(std::move(t));
 *
 * Pitfalls
 * --------
 *   • **Order sensitivity** — `Compress(Encrypt(x))` is not `Encrypt(Compress(x))`.
 *
 *   • **Identity and equality** — two decorated trees may render the same but
 *     compare unequal; document whether wrappers are transparent for `==`.
 *
 *   • **Deep stacks** — many layers add indirection; profile before micro-wrapping
 *     every hot call.
 *
 *   • **Forgotten forwarding** — a new method on `Component` requires updates in
 *     every decorator or a centralized base that forwards by default.
 *
 * Testing
 * -------
 *   • Test each decorator in isolation with a **test double** inner component.
 *   • Add integration tests for a few representative stacks (order, nesting depth).
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

namespace usage_examples::patterns::structural {

	// -----------------------------------------------------------------
	// Text pipeline — decorators wrap ITextSource with unique_ptr chain.
	// -----------------------------------------------------------------
	struct ITextSource
	{
		virtual ~ITextSource() = default;
		[[nodiscard]] virtual std::string content() const = 0;
	};

	struct PlainText final : ITextSource
	{
		std::string value;

		explicit PlainText(std::string s) : value(std::move(s)) {}

		[[nodiscard]] std::string content() const override { return value; }
	};

	struct TextDecorator : ITextSource
	{
		explicit TextDecorator(std::unique_ptr<ITextSource> inner) : inner_(std::move(inner)) {}

	protected:
		[[nodiscard]] const ITextSource& inner() const { return *inner_; }

	private:
		std::unique_ptr<ITextSource> inner_;
	};

	struct StarFrameDecorator final : TextDecorator
	{
		using TextDecorator::TextDecorator;

		[[nodiscard]] std::string content() const override { return "** " + inner().content() + " **"; }
	};

	struct QuoteDecorator final : TextDecorator
	{
		using TextDecorator::TextDecorator;

		[[nodiscard]] std::string content() const override { return "> " + inner().content(); }
	};

	// -----------------------------------------------------------------
	// Priced item — classic beverage-style cumulative cost and description.
	// -----------------------------------------------------------------
	struct IPricedItem
	{
		virtual ~IPricedItem() = default;
		[[nodiscard]] virtual int cents() const = 0;
		[[nodiscard]] virtual std::string description() const = 0;
	};

	struct HouseCoffee final : IPricedItem
	{
		[[nodiscard]] int cents() const override { return 250; }
		[[nodiscard]] std::string description() const override { return "coffee"; }
	};

	struct PricedDecorator : IPricedItem
	{
		explicit PricedDecorator(std::unique_ptr<IPricedItem> inner) : inner_(std::move(inner)) {}

	protected:
		[[nodiscard]] const IPricedItem& inner() const { return *inner_; }

	private:
		std::unique_ptr<IPricedItem> inner_;
	};

	struct MilkDecorator final : PricedDecorator
	{
		using PricedDecorator::PricedDecorator;

		[[nodiscard]] int cents() const override { return inner().cents() + 55; }

		[[nodiscard]] std::string description() const override { return inner().description() + ", milk"; }
	};

	struct SugarDecorator final : PricedDecorator
	{
		using PricedDecorator::PricedDecorator;

		[[nodiscard]] int cents() const override { return inner().cents() + 10; }

		[[nodiscard]] std::string description() const override { return inner().description() + ", sugar"; }
	};

} // namespace usage_examples::patterns::structural

namespace {

	using usage_examples::patterns::structural::HouseCoffee;
	using usage_examples::patterns::structural::MilkDecorator;
	using usage_examples::patterns::structural::PlainText;
	using usage_examples::patterns::structural::QuoteDecorator;
	using usage_examples::patterns::structural::StarFrameDecorator;
	using usage_examples::patterns::structural::SugarDecorator;

	TEST(DecoratorUsageExamples, PlainTextHasNoWrapping)
	{
		const PlainText t("hello");
		EXPECT_EQ(t.content(), "hello");
	}

	TEST(DecoratorUsageExamples, StarFrameWrapsInnerContent)
	{
		auto inner = std::make_unique<PlainText>("go");
		const StarFrameDecorator deco(std::move(inner));
		EXPECT_EQ(deco.content(), "** go **");
	}

	TEST(DecoratorUsageExamples, StackedDecoratorsApplyOutwardInOrder)
	{
		std::unique_ptr<usage_examples::patterns::structural::ITextSource> chain =
			std::make_unique<PlainText>("x");
		chain = std::make_unique<StarFrameDecorator>(std::move(chain));
		chain = std::make_unique<QuoteDecorator>(std::move(chain));
		EXPECT_EQ(chain->content(), "> ** x **");
	}

	TEST(DecoratorUsageExamples, BeverageDecoratorsAddCostAndLabels)
	{
		std::unique_ptr<usage_examples::patterns::structural::IPricedItem> drink =
			std::make_unique<HouseCoffee>();
		drink = std::make_unique<MilkDecorator>(std::move(drink));
		drink = std::make_unique<SugarDecorator>(std::move(drink));
		EXPECT_EQ(drink->cents(), 250 + 55 + 10);
		EXPECT_EQ(drink->description(), "coffee, milk, sugar");
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add a `LoggingDecorator` around an `IStreamSink` that records byte counts
 *    before forwarding — compare with a **Proxy** that only guards access.
 * 2. Implement optional **undo** for decorators that mutate state (harder): keep
 *    a stack of reversible operations or prefer immutable decorated values.
 * 3. Use a small **factory** function `make_text_stack(Config)` so ordering rules
 *    live in one place instead of scattered `make_unique` chains.
 */
