/*
 * =============================================================================
 * Builder (Gang of Four — Creational)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Separate the *construction* of a complex object from its *representation*,
 * so the same construction process can create different representations.
 *
 * Think of a multi-course recipe (the process): you can serve the same steps
 * as a casual plate or a formal tasting menu (different representations). The
 * Builder pattern gives you stepwise control, optional parts, and readable
 * client code without enormous constructors.
 *
 * Typical structure
 * -----------------
 *   • Builder        — abstract interface: `buildPartA()`, `buildPartB()`, …
 *   • ConcreteBuilder — implements steps; often accumulates state; `getResult()`
 *   • Director       — optional: knows *which* steps to call and in what order
 *   • Product        — the complex object (sometimes an opaque type or DTO)
 *
 * The Director is not mandatory. Many C++ codebases use only a fluent
 * `ThingBuilder` with a chain of `with_*` methods and a final `build()`.
 *
 * Why use it
 * ----------
 *   • Many optional fields — avoids "telescoping constructors" (`Foo(a)`, `Foo(a,b)`,
 *     `Foo(a,b,c)`, …) and boolean parameter soup.
 *
 *   • Invariants — validate once in `build()` (or `try_build()` returning
 *     `std::expected`) instead of scattered across setters.
 *
 *   • Readable call sites — `Request::builder().method("GET").path("/x").build()`.
 *
 *   • Same algorithm, different output — e.g. plain-text vs HTML email from one
 *     Director driving two ConcreteBuilders.
 *
 * Builder vs Abstract Factory
 * ----------------------------
 *   • Abstract Factory creates *families* of related products (Button + Checkbox
 *     from one factory).
 *
 *   • Builder assembles *one* complex product step by step; often one Builder
 *     instance corresponds to one product under construction.
 *
 * Builder vs Factory Method
 * ----------------------------
 *   • Factory Method hides *which concrete class* is instantiated (subclass hook).
 *
 *   • Builder focuses on *how* an object is pieced together over several calls,
 *     sometimes with a fluent API and optional steps.
 *
 * Fluent builders in C++
 * ----------------------
 *   • Returning `Builder&` from setters enables chaining:
 *       `FooBuilder{}.set_a(1).set_b(2).build();`
 *
 *   • Returning `Builder&&` (rvalue-ref qualified) can enforce "chain only on
 *     temporaries" in advanced APIs; most code uses `Builder&`.
 *
 *   • Consider `build() const` if the builder is logically a factory read-only
 *     at the end; or non-const if `build()` moves from internal buffers.
 *
 * Validation and failure
 * ----------------------
 *   • Throw `std::invalid_argument` (or domain exception) from `build()` when
 *     required fields are missing.
 *
 *   • C++23 `std::expected<Product, Error>` avoids exceptions for expected
 *     validation failures — good for parsers and user input.
 *
 * Pitfalls
 * --------
 *   • **Reusing one builder** after `build()` without `reset()` — second build
 *     may leak stale state. Document whether builders are single-use.
 *
 *   • **Partially built objects escaping** — keep the product type private until
 *     `build()`, or use a dedicated `IncompleteFoo` only inside the builder.
 *
 *   • **Over-engineering** — a struct with three fields rarely needs a Director;
 *     a few defaulted members or designated initializers may suffice.
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   // Fluent builder (no Director):
 *   class PizzaBuilder {
 *    public:
 *     PizzaBuilder& dough(std::string d) { dough_ = std::move(d); return *this; }
 *     PizzaBuilder& cheese(std::string c) { cheese_ = std::move(c); return *this; }
 *     Pizza build() const {
 *       if (dough_.empty()) throw std::invalid_argument("dough required");
 *       return Pizza{dough_, cheese_};
 *     }
 *    private:
 *     std::string dough_, cheese_;
 *   };
 *
 *   // Director + two concrete builders (same steps, different representation):
 *   void compose_invoice(InvoiceBuilder& b) {
 *     b.reset();
 *     b.add_line("Widget", 2, 19.99);
 *     b.add_tax(0.07);
 *   }
 *   // PlainTextInvoiceBuilder vs PdfInvoiceBuilder produce different strings/PDFs.
 *
 * Testing
 * -------
 *   • Unit-test `build()` validation paths (missing required fields).
 *   • With a Director, test that the same director + different builders yields
 *     structurally different products as intended.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace usage_examples::patterns::behavioral {

	// -----------------------------------------------------------------
	// Example 1 — Fluent builder (Director optional / omitted)
	// Builds an immutable-style HTTP request configuration DTO.
	// -----------------------------------------------------------------
	struct HttpRequest
	{
		std::string method;
		std::string path;
		std::string body;
	};

	class HttpRequestBuilder
	{
	public:
		HttpRequestBuilder& set_method(std::string m)
		{
			method_ = std::move(m);
			return *this;
		}

		HttpRequestBuilder& set_path(std::string p)
		{
			path_ = std::move(p);
			return *this;
		}

		HttpRequestBuilder& set_body(std::string b)
		{
			body_ = std::move(b);
			return *this;
		}

		[[nodiscard]] HttpRequest build() const
		{
			if (method_.empty()) throw std::invalid_argument("method is required");
			if (path_.empty()) throw std::invalid_argument("path is required");
			return HttpRequest{ method_, path_, body_ };
		}

	private:
		std::string method_;
		std::string path_;
		std::string body_;
	};

	// -----------------------------------------------------------------
	// Example 2 — Director + abstract Builder (classic GoF collaboration)
	// Same construction steps; plain vs HTML representation of body.
	// -----------------------------------------------------------------
	struct EmailMessage
	{
		std::string to;
		std::string subject;
		std::string body;
	};

	class EmailBuilder
	{
	public:
		virtual ~EmailBuilder() = default;

		virtual void reset() = 0;
		virtual void set_recipient(std::string to) = 0;
		virtual void set_subject(std::string subject) = 0;
		virtual void set_main_text(std::string text) = 0;
		[[nodiscard]] virtual EmailMessage build() = 0;
	};

	class PlainEmailBuilder final : public EmailBuilder
	{
	public:
		void reset() override
		{
			to_.clear();
			subject_.clear();
			main_.clear();
		}

		void set_recipient(std::string to) override { to_ = std::move(to); }

		void set_subject(std::string subject) override { subject_ = std::move(subject); }

		void set_main_text(std::string text) override { main_ = std::move(text); }

		[[nodiscard]] EmailMessage build() override
		{
			return EmailMessage{ to_, subject_, main_ };
		}

	private:
		std::string to_;
		std::string subject_;
		std::string main_;
	};

	class HtmlEmailBuilder final : public EmailBuilder
	{
	public:
		void reset() override
		{
			to_.clear();
			subject_.clear();
			main_.clear();
		}

		void set_recipient(std::string to) override { to_ = std::move(to); }

		void set_subject(std::string subject) override { subject_ = std::move(subject); }

		void set_main_text(std::string text) override { main_ = std::move(text); }

		[[nodiscard]] EmailMessage build() override
		{
			// Deliberately tiny "HTML" — illustrates different representation.
			std::string html = "<html><body><h1>" + subject_ + "</h1><p>" + main_ + "</p></body></html>";
			return EmailMessage{ to_, subject_, std::move(html) };
		}

	private:
		std::string to_;
		std::string subject_;
		std::string main_;
	};

	// Director: encodes the *recipe* for a welcome email (order of steps).
	inline void construct_welcome_email(EmailBuilder& builder)
	{
		builder.reset();
		builder.set_recipient("pat@example.com");
		builder.set_subject("Welcome");
		builder.set_main_text("Your account is ready.");
	}

	// -----------------------------------------------------------------
	// Example 3 — Incremental assembly with explicit reset semantics
	// (useful when reusing builder instances in a loop).
	// -----------------------------------------------------------------
	struct QueryFilter
	{
		std::string where_clause;
		int limit = 0;
	};

	class QueryFilterBuilder
	{
	public:
		QueryFilterBuilder& where_equals(std::string column, std::string value)
		{
			if (!where_.empty()) where_ += " AND ";
			where_ += std::move(column) + " = '" + std::move(value) + "'";
			return *this;
		}

		QueryFilterBuilder& take(int n)
		{
			limit_ = n;
			return *this;
		}

		void clear()
		{
			where_.clear();
			limit_ = 0;
		}

		[[nodiscard]] QueryFilter build() const
		{
			return QueryFilter{ where_, limit_ };
		}

	private:
		std::string where_;
		int limit_ = 0;
	};

} // namespace usage_examples::patterns::behavioral

namespace {

	using usage_examples::patterns::behavioral::construct_welcome_email;
	using usage_examples::patterns::behavioral::EmailMessage;
	using usage_examples::patterns::behavioral::HtmlEmailBuilder;
	using usage_examples::patterns::behavioral::HttpRequestBuilder;
	using usage_examples::patterns::behavioral::PlainEmailBuilder;
	using usage_examples::patterns::behavioral::QueryFilterBuilder;

	TEST(BuilderUsageExamples, FluentHttpRequestBuilderChainsAndValidates)
	{
		const auto req = HttpRequestBuilder{}
			.set_method("POST")
			.set_path("/api/items")
			.set_body(R"({"name":"cup"})")
			.build();

		EXPECT_EQ(req.method, "POST");
		EXPECT_EQ(req.path, "/api/items");
		EXPECT_EQ(req.body, R"({"name":"cup"})");
	}

	TEST(BuilderUsageExamples, HttpRequestBuilderThrowsWhenRequiredFieldsMissing)
	{
		EXPECT_THROW((void)HttpRequestBuilder{}.set_method("GET").build(), std::invalid_argument);
		EXPECT_THROW((void)HttpRequestBuilder{}.set_path("/").build(), std::invalid_argument);
	}

	TEST(BuilderUsageExamples, DirectorWithPlainBuilderKeepsPlainBody)
	{
		PlainEmailBuilder builder;
		construct_welcome_email(builder);
		const EmailMessage msg = builder.build();

		EXPECT_EQ(msg.to, "pat@example.com");
		EXPECT_EQ(msg.subject, "Welcome");
		EXPECT_EQ(msg.body, "Your account is ready.");
	}

	TEST(BuilderUsageExamples, DirectorWithHtmlBuilderWrapsBody)
	{
		HtmlEmailBuilder builder;
		construct_welcome_email(builder);
		const EmailMessage msg = builder.build();

		EXPECT_EQ(msg.to, "pat@example.com");
		EXPECT_EQ(msg.subject, "Welcome");
		EXPECT_NE(msg.body.find("<h1>Welcome</h1>"), std::string::npos);
		EXPECT_NE(msg.body.find("<p>Your account is ready.</p>"), std::string::npos);
	}

	TEST(BuilderUsageExamples, QueryFilterBuilderSupportsReuseAfterClear)
	{
		QueryFilterBuilder qb;
		qb.where_equals("status", "open").take(10);
		EXPECT_EQ(qb.build().where_clause, "status = 'open'");
		EXPECT_EQ(qb.build().limit, 10);

		qb.clear();
		qb.where_equals("owner", "ada").take(3);
		const auto f = qb.build();
		EXPECT_EQ(f.where_clause, "owner = 'ada'");
		EXPECT_EQ(f.limit, 3);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add `try_build() -> std::expected<HttpRequest, std::string>` beside `build()`
 *    and map validation errors without exceptions.
 * 2. Implement a `Memento`-style snapshot of builder state if you need undo.
 * 3. Combine Builder with Abstract Factory: a factory returns a preconfigured
 *    builder for a given product line (e.g. "enterprise" vs "starter" presets).
 */
