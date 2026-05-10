/*
 * =============================================================================
 * Strategy (Gang of Four — Behavioral)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Define a **family of algorithms**, **encapsulate** each one, and make them
 * **interchangeable**. Strategy lets the algorithm **vary independently** from
 * clients that use it.
 *
 * The **Context** object performs work by delegating to a **Strategy** interface.
 * Concrete strategies supply different behaviors (pricing rules, compression
 * codecs, layout engines) without editing the Context for each variant.
 *
 * Typical structure
 * -----------------
 *   • **Strategy** — common interface (`compute`, `encode`, `compare`).
 *   • **ConcreteStrategy** — algorithm implementation A, B, C.
 *   • **Context** — holds a Strategy (pointer, reference, or `std::function`),
 *     exposes domain operations that forward to the current strategy.
 *
 * Why use it
 * ----------
 *   • **Open/Closed** — add a new pricing rule by new class, not by growing a
 *     `switch (customer_tier)` in ten places.
 *
 *   • **Testability** — inject a fake or deterministic strategy when testing the
 *     Context.
 *
 *   • **Runtime choice** — user settings, A/B flags, or configuration files pick
 *     the strategy without recompiling.
 *
 * Strategy vs Template Method
 * ---------------------------
 *   • **Template Method** fixes an **algorithm skeleton** in a base class and
 *     lets **subclasses** override specific steps (inheritance, `virtual` hooks).
 *
 *   • **Strategy** **composes in** a whole algorithm object; families vary by
 *     **replacing** the strategy instance, often without subclassing the Context.
 *
 * Strategy vs State
 * -----------------
 *   • **State** objects usually **drive transitions** (“when event X, move to
 *     State B”) and the *meaning* of operations changes with the current state.
 *
 *   • **Strategy** choices are often **orthogonal policies** selected by config;
 *     the Context does not model a state machine unless you blend patterns.
 *
 * Strategy vs Bridge (reminder)
 * -----------------------------
 *   • **Bridge** splits a **large abstraction** from a **platform-sized**
 *     implementation hierarchy (windowing vs rendering).
 *
 *   • **Strategy** is usually a **smaller swappable policy** inside a Context
 *     (“how to price,” “how to compare”). Boundaries blur in real code — intent
 *     and naming matter.
 *
 * C++ idioms
 * ----------
 *   • **`std::unique_ptr<Strategy>`** owned by Context when strategies are
 *     polymorphic classes.
 *
 *   • **`std::function<R(Args...)>`** as a **lightweight strategy** for one or
 *     two call sites — great for lambdas; worse for large families needing
 *     virtual tables and shared state inside strategy objects.
 *
 *   • **Type-erased** strategies (function objects + small buffer) avoid virtual
 *     calls when profiling demands it — advanced and easy to misuse.
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   // Functional strategy:
 *   using HashFn = std::function<std::size_t(std::string_view)>;
 *   struct DocumentIndex {
 *     HashFn hash = [](std::string_view s) { return std::hash<std::string_view>{}(s); };
 *   };
 *
 *   // Enum + switch is a primitive strategy selector — fine until branches
 *   // explode; then extract Strategy classes or a map<string, Factory>.
 *
 * Pitfalls
 * --------
 *   • **Leaking strategy details** through the Context API — keep the Strategy
 *     interface minimal (Interface Segregation).
 *
 *   • **Null strategy** — document whether Context always has a valid strategy
 *     or provide a safe default.
 *
 *   • **Lifetime** — Context must not outlive a Strategy stored by reference
 *     unless ownership is clear (`shared_ptr`, arena).
 *
 * Testing
 * -------
 *   • Test each ConcreteStrategy in isolation with pure inputs.
 *   • Test Context with a **recording strategy** that captures arguments.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <cctype>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace usage_examples::patterns::behavioral {

	// -----------------------------------------------------------------
	// Strategy objects — interchangeable pricing rules.
	// -----------------------------------------------------------------
	struct IDiscountStrategy
	{
		virtual ~IDiscountStrategy() = default;
		[[nodiscard]] virtual int discounted_total_cents(int subtotal_cents) const = 0;
	};

	struct NoDiscountStrategy final : IDiscountStrategy
	{
		[[nodiscard]] int discounted_total_cents(int subtotal_cents) const override { return subtotal_cents; }
	};

	struct PercentOffStrategy final : IDiscountStrategy
	{
		explicit PercentOffStrategy(int percent_0_to_100) : pct_(percent_0_to_100) {}

		[[nodiscard]] int discounted_total_cents(int subtotal_cents) const override
		{
			if (pct_ <= 0) return subtotal_cents;
			if (pct_ >= 100) return 0;
			return subtotal_cents - (subtotal_cents * pct_ / 100);
		}

	private:
		int pct_;
	};

	struct FixedOffStrategy final : IDiscountStrategy
	{
		explicit FixedOffStrategy(int off_cents) : off_(off_cents) {}

		[[nodiscard]] int discounted_total_cents(int subtotal_cents) const override
		{
			const int v = subtotal_cents - off_;
			return v < 0 ? 0 : v;
		}

	private:
		int off_;
	};

	// -----------------------------------------------------------------
	// Context — checkout owns the current strategy and delegates pricing.
	// -----------------------------------------------------------------
	class CheckoutCart
	{
	public:
		explicit CheckoutCart(std::unique_ptr<IDiscountStrategy> strategy) : strategy_(std::move(strategy)) {}

		void set_discount_policy(std::unique_ptr<IDiscountStrategy> strategy) { strategy_ = std::move(strategy); }

		[[nodiscard]] int quote_total_cents(int merchandise_subtotal_cents) const
		{
			return strategy_->discounted_total_cents(merchandise_subtotal_cents);
		}

	private:
		std::unique_ptr<IDiscountStrategy> strategy_;
	};

	// -----------------------------------------------------------------
	// Lightweight strategy via std::function — text normalization pipeline.
	// -----------------------------------------------------------------
	class TextNormalizer
	{
	public:
		using Transform = std::function<std::string(std::string_view)>;

		explicit TextNormalizer(Transform fn) : transform_(std::move(fn)) {}

		void set_transform(Transform fn) { transform_ = std::move(fn); }

		[[nodiscard]] std::string normalize(std::string_view input) const { return transform_(input); }

	private:
		Transform transform_;
	};

	inline std::string to_upper_copy(std::string_view s)
	{
		std::string out;
		out.reserve(s.size());
		for (unsigned char c : s) out.push_back(static_cast<char>(std::toupper(c)));
		return out;
	}

} // namespace usage_examples::patterns::behavioral

namespace {

	using usage_examples::patterns::behavioral::CheckoutCart;
	using usage_examples::patterns::behavioral::FixedOffStrategy;
	using usage_examples::patterns::behavioral::NoDiscountStrategy;
	using usage_examples::patterns::behavioral::PercentOffStrategy;
	using usage_examples::patterns::behavioral::TextNormalizer;
	using usage_examples::patterns::behavioral::to_upper_copy;

	TEST(StrategyUsageExamples, CheckoutDelegatesToSelectedDiscountPolicy)
	{
		CheckoutCart cart(std::make_unique<NoDiscountStrategy>());
		EXPECT_EQ(cart.quote_total_cents(1000), 1000);

		cart.set_discount_policy(std::make_unique<PercentOffStrategy>(20));
		EXPECT_EQ(cart.quote_total_cents(1000), 800);

		cart.set_discount_policy(std::make_unique<FixedOffStrategy>(350));
		EXPECT_EQ(cart.quote_total_cents(1000), 650);
	}

	TEST(StrategyUsageExamples, PercentOffNeverReturnsNegative)
	{
		CheckoutCart cart(std::make_unique<PercentOffStrategy>(100));
		EXPECT_EQ(cart.quote_total_cents(999), 0);
	}

	TEST(StrategyUsageExamples, TextNormalizerUsesStdFunctionStrategy)
	{
		TextNormalizer pipe([](std::string_view s) { return std::string(s); });
		EXPECT_EQ(pipe.normalize("abc"), "abc");

		pipe.set_transform(to_upper_copy);
		EXPECT_EQ(pipe.normalize("aBc"), "ABC");
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Register strategies by name in `std::unordered_map<std::string,
 *    std::function<std::unique_ptr<IDiscountStrategy>()>>` for config-driven
 *    pricing plugins.
 * 2. Combine with **Decorator**: wrap a `IDiscountStrategy` in
 *    `LoggingDiscountDecorator` without changing `CheckoutCart`.
 * 3. Contrast a **CRTP** policy host (`Host<PercentPolicy>`) with this runtime
 *    Strategy — when would you pick compile-time dispatch instead?
 */
