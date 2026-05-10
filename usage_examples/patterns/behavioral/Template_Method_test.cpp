/*
 * =============================================================================
 * Template Method (Gang of Four — Behavioral)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Define the **skeleton of an algorithm** in an operation, deferring **some
 * steps** to subclasses. Template Method lets subclasses **redefine certain
 * steps** of an algorithm **without changing** the algorithm’s structure.
 *
 * The fixed sequence lives in a **non-virtual public method** (the *template
 * method*). Variation happens in **protected virtual** “primitive operations”
 * or **hooks** that the base calls in a prescribed order.
 *
 * (Do not confuse this pattern with C++ **templates**: the name refers to a
 * *method that serves as a behavioral template*, not to the language feature.)
 *
 * Typical structure
 * -----------------
 *   • **AbstractClass** — declares `algorithm()` (often non-virtual) that calls
 *     `step1()`, `step2()`, `hook3()`, … in order. Implements **default**
 *     behavior for some steps; marks others `= 0` or `virtual` with a body.
 *
 *   • **ConcreteClass** — overrides selected steps to specialize behavior while
 *     inheriting the **overall control flow** from the base.
 *
 * Non-Virtual Interface (NVI) idiom
 * ---------------------------------
 * In C++, the template method is usually **`public` non-virtual** `run()` while
 * customization points are **`private` or `protected` `virtual`** helpers. That
 * lets the base class **enforce invariants** (logging, locks, validation)
 * around calls to `do_step()` without subclasses skipping the wrapper.
 *
 * Hooks vs abstract primitives
 * ----------------------------
 *   • **Abstract primitive** — base has **no** sensible default; subclass must
 *     implement (`= 0`).
 *
 *   • **Hook** — base provides a **do-nothing or minimal** default; subclasses
 *     override **only when needed** (`virtual void on_complete() {}`).
 *
 * This mirrors **final** classes in frameworks: “you *must* implement X, you
 * *may* override Y.”
 *
 * Template Method vs Strategy
 * ---------------------------
 *   • **Template Method** — algorithm structure is **fixed in a base class**;
 *     variation through **inheritance** and **overriding** steps.
 *
 *   • **Strategy** — algorithm family is **pluggable by composition**; the
 *     context holds a **Strategy** object and delegates without subclassing the
 *     context.
 *
 * Use Template Method when **one family** of algorithms shares a **stable
 * sequence**; use Strategy when you want to **swap** entire behaviors at runtime
 * or avoid deep inheritance trees.
 *
 * Benefits and costs
 * ------------------
 *   • **Reuse** — duplicate control flow is written **once**; fewer “copy-paste
 *     loops” across similar services.
 *
 *   • **Hollywood Principle** — “don’t call us, we’ll call you”: the base
 *     drives the flow and invokes subclass hooks at the right times.
 *
 *   • **Fragile base class** — changing the sequence in the base can **break**
 *     subclasses that relied on old ordering; prefer **documented** extension
 *     points and semantic versioning.
 *
 *   • **Deep hierarchies** — many layers of overrides become hard to reason
 *     about; sometimes **Strategy**, **composition**, or **small free functions**
 *     scale better.
 *
 * C++ implementation notes
 * ------------------------
 *   • Mark the template method **`final`** if subclasses must not replace the
 *     whole algorithm (only the steps).
 *
 *   • **`virtual` destructor** in polymorphic bases so `delete` through base
 *     pointers is safe when you store exporters/beverages behind `unique_ptr`.
 *
 *   • **CRTP** can “static template method” at compile time (`Derived::step()`)
 *     with **no virtuals** — different trade-off (monomorphization, no runtime
 *     substitution).
 *
 *   • **Coroutines / ranges** can express pipelines; you still may want a class
 *     that **documents** the required order of stages for a product feature.
 *
 * Testing
 * -------
 *   • **Trace vectors** — record step names in the base and assert **order** for
 *     each concrete type.
 *
 *   • **Golden strings** — snapshot exporter output for regression tests.
 *
 *   • **Hook coverage** — subclass that only overrides hooks ensures defaults
 *     do not break when hooks are optional.
 *
 *   • **One-definition rule** — demo types live in `template_method_gof` so
 *     this file can coexist with other `usage_examples` sources.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace usage_examples::patterns::behavioral::template_method_gof {

	// -----------------------------------------------------------------
	// Example 1 — Beverage preparation (classic “coffee vs tea” story)
	// -----------------------------------------------------------------
	class HotBeverage
	{
	public:
		virtual ~HotBeverage() = default;

		// Template method: fixed sequence; subclasses customize brew + extras.
		void prepare(std::vector<std::string>& trace) const
		{
			record(trace, "boil_water");
			brew(trace);
			record(trace, "pour_in_cup");
			add_condiments(trace);
		}

	protected:
		static void record(std::vector<std::string>& trace, std::string_view step)
		{
			trace.emplace_back(std::string(step));
		}

		virtual void brew(std::vector<std::string>& trace) const = 0;
		virtual void add_condiments(std::vector<std::string>& trace) const = 0;
	};

	class DemoCoffee final : public HotBeverage
	{
	protected:
		void brew(std::vector<std::string>& trace) const override { record(trace, "brew_coffee_grinds"); }

		void add_condiments(std::vector<std::string>& trace) const override
		{
			record(trace, "add_sugar_and_milk");
		}
	};

	class DemoTea final : public HotBeverage
	{
	protected:
		void brew(std::vector<std::string>& trace) const override { record(trace, "steep_tea_bag"); }

		void add_condiments(std::vector<std::string>& trace) const override { record(trace, "add_lemon"); }
	};

	// -----------------------------------------------------------------
	// Example 2 — Mini report exporter: same pipeline, different formats
	// -----------------------------------------------------------------
	class MiniReportExporter
	{
	public:
		virtual ~MiniReportExporter() = default;

		[[nodiscard]] std::string export_sales_stub() const
		{
			std::string out;
			open_document(out);
			write_title(out, "Daily sales");
			write_row(out, "sku-12", "4");
			write_row(out, "sku-99", "1");
			close_document(out);
			return out;
		}

	protected:
		virtual void open_document(std::string& out) const = 0;
		virtual void write_title(std::string& out, std::string_view title) const = 0;
		virtual void write_row(std::string& out, std::string_view key, std::string_view value) const = 0;

		// Hook: default appends newline for readability in tests.
		virtual void close_document(std::string& out) const { out += "\n"; }
	};

	class JsonMiniExporter final : public MiniReportExporter
	{
	protected:
		void open_document(std::string& out) const override { out = R"({"title":")"; }

		void write_title(std::string& out, std::string_view title) const override
		{
			out.append(title);
			out += R"(","rows":[)";
		}

		void write_row(std::string& out, std::string_view key, std::string_view value) const override
		{
			if (out.back() != '[') out += ',';
			out += R"({"k":")";
			out.append(key);
			out += R"(","v":")";
			out.append(value);
			out += R"("})";
		}

		void close_document(std::string& out) const override
		{
			out += "]}";
			MiniReportExporter::close_document(out);
		}
	};

	class XmlMiniExporter final : public MiniReportExporter
	{
	protected:
		void open_document(std::string& out) const override { out = "<report>"; }

		void write_title(std::string& out, std::string_view title) const override
		{
			out += "<title>";
			out.append(title);
			out += "</title><rows>";
		}

		void write_row(std::string& out, std::string_view key, std::string_view value) const override
		{
			out += "<row><key>";
			out.append(key);
			out += "</key><qty>";
			out.append(value);
			out += "</qty></row>";
		}

		void close_document(std::string& out) const override
		{
			out += "</rows></report>";
			MiniReportExporter::close_document(out);
		}
	};

	// -----------------------------------------------------------------
	// Example 3 — Numeric pipeline with optional preprocess / postprocess hooks
	// -----------------------------------------------------------------
	class NumberPipeline
	{
	public:
		virtual ~NumberPipeline() = default;

		[[nodiscard]] int run(int x) const
		{
			x = preprocess(x);
			x = transform(x);
			return postprocess(x);
		}

	protected:
		virtual int preprocess(int x) const { return x; }
		virtual int transform(int x) const = 0;
		virtual int postprocess(int x) const { return x; }
	};

	class DoublePipeline final : public NumberPipeline
	{
	protected:
		int transform(int x) const override { return x * 2; }
	};

	class ClampThenTriplePipeline final : public NumberPipeline
	{
	protected:
		int preprocess(int x) const override { return (x < 0) ? 0 : x; }

		int transform(int x) const override { return x * 3; }

		int postprocess(int x) const override { return x + 10; }
	};

	// -----------------------------------------------------------------
	// Example 4 — NVI-style “final” template method with private virtual hooks
	// -----------------------------------------------------------------
	class AuditCounter
	{
	public:
		virtual ~AuditCounter() = default;

		void increment_both(int& a, int& b)
		{
			before_tick(a, b);
			do_increment(a, b);
			after_tick(a, b);
		}

	protected:
		virtual void before_tick(int& a, int& b) { (void)a; (void)b; }
		virtual void do_increment(int& a, int& b) = 0;
		virtual void after_tick(int& a, int& b) { (void)a; (void)b; }
	};

	class PlainAddOne final : public AuditCounter
	{
		void do_increment(int& a, int& b) override
		{
			++a;
			++b;
		}
	};

	class LoggedAddOne final : public AuditCounter
	{
	public:
		int before_calls = 0;
		int after_calls = 0;

	protected:
		void before_tick(int& a, int& b) override
		{
			++before_calls;
			(void)a;
			(void)b;
		}

		void do_increment(int& a, int& b) override
		{
			++a;
			++b;
		}

		void after_tick(int& a, int& b) override
		{
			++after_calls;
			(void)a;
			(void)b;
		}
	};

} // namespace usage_examples::patterns::behavioral::template_method_gof

namespace {

	using usage_examples::patterns::behavioral::template_method_gof::ClampThenTriplePipeline;
	using usage_examples::patterns::behavioral::template_method_gof::DemoCoffee;
	using usage_examples::patterns::behavioral::template_method_gof::DemoTea;
	using usage_examples::patterns::behavioral::template_method_gof::DoublePipeline;
	using usage_examples::patterns::behavioral::template_method_gof::JsonMiniExporter;
	using usage_examples::patterns::behavioral::template_method_gof::LoggedAddOne;
	using usage_examples::patterns::behavioral::template_method_gof::MiniReportExporter;
	using usage_examples::patterns::behavioral::template_method_gof::PlainAddOne;
	using usage_examples::patterns::behavioral::template_method_gof::XmlMiniExporter;

	TEST(TemplateMethodUsageExamples, CoffeePrepareSequence)
	{
		DemoCoffee coffee;
		std::vector<std::string> trace;
		coffee.prepare(trace);
		ASSERT_EQ(trace.size(), 4u);
		EXPECT_EQ(trace[0], "boil_water");
		EXPECT_EQ(trace[1], "brew_coffee_grinds");
		EXPECT_EQ(trace[2], "pour_in_cup");
		EXPECT_EQ(trace[3], "add_sugar_and_milk");
	}

	TEST(TemplateMethodUsageExamples, TeaPrepareSequence)
	{
		DemoTea tea;
		std::vector<std::string> trace;
		tea.prepare(trace);
		EXPECT_EQ(trace[1], "steep_tea_bag");
		EXPECT_EQ(trace[3], "add_lemon");
	}

	TEST(TemplateMethodUsageExamples, PolymorphicExporterThroughBasePointer)
	{
		std::unique_ptr<MiniReportExporter> ex = std::make_unique<JsonMiniExporter>();
		const std::string json = ex->export_sales_stub();
		EXPECT_NE(json.find(R"("sku-12")"), std::string::npos);
		EXPECT_NE(json.find(R"("Daily sales")"), std::string::npos);

		ex = std::make_unique<XmlMiniExporter>();
		const std::string xml = ex->export_sales_stub();
		EXPECT_NE(xml.find("<title>Daily sales</title>"), std::string::npos);
		EXPECT_NE(xml.find("sku-99"), std::string::npos);
	}

	TEST(TemplateMethodUsageExamples, NumberPipelineHooks)
	{
		DoublePipeline d;
		EXPECT_EQ(d.run(5), 10);

		ClampThenTriplePipeline c;
		EXPECT_EQ(c.run(-5), 10);
		EXPECT_EQ(c.run(2), 16);
	}

	TEST(TemplateMethodUsageExamples, NviCounterCallsHooksAroundPrimitive)
	{
		int x = 0;
		int y = 0;
		PlainAddOne plain;
		plain.increment_both(x, y);
		EXPECT_EQ(x, 1);
		EXPECT_EQ(y, 1);

		LoggedAddOne logged;
		x = y = 0;
		logged.increment_both(x, y);
		EXPECT_EQ(logged.before_calls, 1);
		EXPECT_EQ(logged.after_calls, 1);
		EXPECT_EQ(x, 1);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Wrap `prepare()` / `export_sales_stub()` with **RAII timing** in the base
 *    (template method logs duration without subclass cooperation).
 * 2. Add a **default hook** `virtual void on_spill(std::vector<std::string>&) {}`
 *    and override it only in `DemoTea` to simulate an optional extension point.
 * 3. Replace virtuals with **CRTP** `template <class Derived> class HotBeverageCrtp`
 *    and measure codegen differences on a hot path.
 * 4. Split `MiniReportExporter` **row iteration** from formatting by injecting a
 *    `RowSource` interface while keeping the open/title/close template method.
 */
