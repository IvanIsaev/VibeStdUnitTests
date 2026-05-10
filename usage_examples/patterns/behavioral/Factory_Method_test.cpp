/*
 * =============================================================================
 * Factory Method (Gang of Four — Creational / Behavioral hook)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Define an interface for creating an object, but let *subclasses* decide
 * which class to instantiate. Factory Method lets a class defer instantiation
 * to subclasses.
 *
 * The pattern is usually drawn with:
 *   • Product           — interface or abstract base for things you build
 *   • ConcreteProduct   — one implementation of Product
 *   • Creator           — declares factory method (often protected/virtual)
 *   • ConcreteCreator   — overrides factory method to return a ConcreteProduct
 *
 * A Creator often calls its own factory method from *template methods* inside
 * the same class: the public algorithm is fixed, but the "hook" that supplies
 * a fresh Product is overridden per subclass. That coupling (inheritance-based
 * extension) is the hallmark of classic Factory Method.
 *
 * Why use it
 * ----------
 *   • You want to follow the Open/Closed Principle: add new product types by
 *     subclassing Creator, without editing existing Creator code that consumes
 *     the Product interface.
 *   • Construction is non-trivial or should stay centralized so callers avoid
 *     knowing concrete class names.
 *   • You need polymorphic factories: the same client code works with any
 *     ConcreteCreator supplied at runtime (dependency injection, plugins).
 *
 * Factory Method vs other "factory" names
 * ---------------------------------------
 *   • Simple Factory (not in GoF): one function or static method that switches
 *     on a parameter (if/else, switch) to `new` the right type. Easy, but each
 *     new type often edits the central switch — less open for extension.
 *
 *   • Factory Method (GoF): the *subclass* provides the concrete type by
 *     overriding a virtual (or CRTP) hook. Adding a type means adding a class,
 *     not editing a global dispatcher (when done cleanly).
 *
 *   • Abstract Factory: builds *families* of related products (e.g. WinTheme
 *     vs MacTheme each creates matching Button + ScrollBar). Factory Method
 *     usually produces *one* kind of Product per Creator subclass.
 *
 * C++ implementation notes
 * ------------------------
 *   • Return `std::unique_ptr<Product>` from the factory method so ownership
 *     is clear and callers do not raw-`delete`. The destructor of Product
 *     should be `virtual` when deleting through base pointer.
 *
 *   • Alternatively return `std::shared_ptr` if ownership is truly shared;
 *     default to `unique_ptr` for sole ownership.
 *
 *   • "Named constructors" / static factory functions on the Product are a
 *     related idiom: they are not subclass-based Factory Method, but they
 *     encapsulate construction and read well: `Color::from_hex("#ff00aa")`.
 *
 *   • Registration maps (`std::unordered_map` of key → creator lambda) are a
 *     *parameterized* factory often used for plugins. They trade compile-time
 *     safety for runtime extensibility; document keys and lifetimes carefully.
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   // Classic virtual factory method on Creator:
 *   class Creator {
 *    public:
 *     void run() {
 *       auto p = create();  // uses subclass hook
 *       p->work();
 *     }
 *    protected:
 *     virtual std::unique_ptr<Product> create() = 0;
 *   };
 *
 *   class ConcreteCreatorX : public Creator {
 *    protected:
 *     std::unique_ptr<Product> create() override {
 *       return std::make_unique<ProductX>();
 *     }
 *   };
 *
 *   // Static factory (named constructor) — no Creator subclass:
 *   class Angle {
 *    public:
 *     static Angle from_degrees(double d) { return Angle(d * kPi / 180.0); }
 *     static Angle from_radians(double r) { return Angle(r); }
 *    private:
 *     explicit Angle(double radians) : rad_(radians) {}
 *     double rad_;
 *   };
 *
 *   // Parameterized registry (plugin-style):
 *   std::unordered_map<std::string, std::function<std::unique_ptr<Plugin>()>> reg;
 *   reg.emplace("jpeg", [] { return std::make_unique<JpegPlugin>(); });
 *   auto p = reg.at("jpeg")();
 *
 * Variations
 * ----------
 *   • Default implementation in base Creator that returns a baseline Product,
 *     with selective overrides in subclasses.
 *
 *   • Factory method parameterized by enum or string — still useful, but
 *     document whether new values require editing one switch (Simple Factory)
 *     or registering new handlers (registry).
 *
 *   • Generic factories with templates: `template<class T> std::unique_ptr<T>
 *     create()` — powerful, but not the classic GoF polymorphic hook; combine
 *     with concepts (C++20) for constraints.
 *
 * Pitfalls
 * --------
 *   • Non-virtual destructor on Product base → undefined behavior when
 *     deleting derived through `unique_ptr<Product>`.
 *   • Over-using inheritance where a small function or lambda would suffice —
 *     not every `make_*` needs a Creator class hierarchy.
 *   • Registry maps: typos in keys, duplicate registration, and static init
 *     order across translation units — prefer explicit init functions or
 *     Meyers singleton for the registry if needed.
 *
 * Testing
 * -------
 * Prefer tests that exercise the *Creator's* public behavior (template method
 * that internally calls the factory) so subclass hooks stay encapsulated.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace usage_examples::patterns::behavioral {

	// -----------------------------------------------------------------
	// Example 1 — Classic Factory Method (GoF structure)
	// Product hierarchy + Creator with protected virtual factory hook.
	// -----------------------------------------------------------------
	struct Document
	{
		virtual ~Document() = default;
		[[nodiscard]] virtual std::string extension() const = 0;
		[[nodiscard]] virtual std::string describe() const = 0;
	};

	struct PdfDocument final : Document
	{
		[[nodiscard]] std::string extension() const override { return ".pdf"; }
		[[nodiscard]] std::string describe() const override { return "pdf"; }
	};

	struct MarkdownDocument final : Document
	{
		[[nodiscard]] std::string extension() const override { return ".md"; }
		[[nodiscard]] std::string describe() const override { return "markdown"; }
	};

	struct DocumentCreator
	{
		virtual ~DocumentCreator() = default;

		// Template-method style: client calls this; hook supplies concrete doc.
		[[nodiscard]] std::string save_stub(const std::string& basename) const
		{
			const auto doc = create_document();
			return basename + doc->extension();
		}

	protected:
		[[nodiscard]] virtual std::unique_ptr<Document> create_document() const = 0;
	};

	struct PdfDocumentCreator final : DocumentCreator
	{
	protected:
		[[nodiscard]] std::unique_ptr<Document> create_document() const override
		{
			return std::make_unique<PdfDocument>();
		}
	};

	struct MarkdownDocumentCreator final : DocumentCreator
	{
	protected:
		[[nodiscard]] std::unique_ptr<Document> create_document() const override
		{
			return std::make_unique<MarkdownDocument>();
		}
	};

	// -----------------------------------------------------------------
	// Example 2 — Named constructors (static factory methods on Product)
	// Not GoF Factory Method, but the same motivation: hide ctor / variants.
	// -----------------------------------------------------------------
	struct RgbColor
	{
		[[nodiscard]] static RgbColor from_bytes(unsigned char r, unsigned char g, unsigned char b)
		{
			return RgbColor{ r, g, b };
		}

		[[nodiscard]] static RgbColor grayscale_byte(unsigned char v) { return RgbColor{ v, v, v }; }

		[[nodiscard]] unsigned char r() const { return r_; }
		[[nodiscard]] unsigned char g() const { return g_; }
		[[nodiscard]] unsigned char b() const { return b_; }

	private:
		RgbColor(unsigned char r, unsigned char g, unsigned char b) : r_(r), g_(g), b_(b) {}

		unsigned char r_, g_, b_;
	};

	// -----------------------------------------------------------------
	// Example 3 — Parameterized registry factory (plugin / key → creator)
	// Runtime dispatch; extend by registering new lambdas.
	// -----------------------------------------------------------------
	enum class TransportKind
	{
		Tcp,
		Udp,
	};

	struct Transport
	{
		virtual ~Transport() = default;
		[[nodiscard]] virtual TransportKind kind() const = 0;
	};

	struct TcpTransport final : Transport
	{
		[[nodiscard]] TransportKind kind() const override { return TransportKind::Tcp; }
	};

	struct UdpTransport final : Transport
	{
		[[nodiscard]] TransportKind kind() const override { return TransportKind::Udp; }
	};

	using TransportFactoryFn = std::function<std::unique_ptr<Transport>()>;

	class TransportRegistry
	{
	public:
		void register_kind(const std::string& key, TransportFactoryFn fn)
		{
			factories_[key] = std::move(fn);
		}

		[[nodiscard]] std::unique_ptr<Transport> create(const std::string& key) const
		{
			const auto it = factories_.find(key);
			if (it == factories_.end()) throw std::out_of_range("unknown transport: " + key);
			return it->second();
		}

	private:
		std::unordered_map<std::string, TransportFactoryFn> factories_;
	};

	inline TransportRegistry make_default_transport_registry()
	{
		TransportRegistry r;
		r.register_kind("tcp", [] { return std::make_unique<TcpTransport>(); });
		r.register_kind("udp", [] { return std::make_unique<UdpTransport>(); });
		return r;
	}

} // namespace usage_examples::patterns::behavioral

namespace {

	using usage_examples::patterns::behavioral::make_default_transport_registry;
	using usage_examples::patterns::behavioral::MarkdownDocumentCreator;
	using usage_examples::patterns::behavioral::PdfDocumentCreator;
	using usage_examples::patterns::behavioral::RgbColor;
	using usage_examples::patterns::behavioral::TransportKind;

	TEST(FactoryMethodUsageExamples, PdfCreatorUsesFactoryHookForExtension)
	{
		const PdfDocumentCreator creator;
		EXPECT_EQ(creator.save_stub("report"), "report.pdf");
	}

	TEST(FactoryMethodUsageExamples, MarkdownCreatorUsesDifferentConcreteProduct)
	{
		const MarkdownDocumentCreator creator;
		EXPECT_EQ(creator.save_stub("notes"), "notes.md");
	}

	TEST(FactoryMethodUsageExamples, NamedConstructorsEncodeIntentWithoutSubclassing)
	{
		const auto red = RgbColor::from_bytes(255, 0, 0);
		EXPECT_EQ(red.r(), 255);
		EXPECT_EQ(red.g(), 0);
		EXPECT_EQ(red.b(), 0);

		const auto gray = RgbColor::grayscale_byte(200);
		EXPECT_EQ(gray.r(), 200);
		EXPECT_EQ(gray.g(), 200);
		EXPECT_EQ(gray.b(), 200);
	}

	TEST(FactoryMethodUsageExamples, RegistryDispatchesByStringKey)
	{
		const auto reg = make_default_transport_registry();
		const auto tcp = reg.create("tcp");
		const auto udp = reg.create("udp");
		EXPECT_EQ(tcp->kind(), TransportKind::Tcp);
		EXPECT_EQ(udp->kind(), TransportKind::Udp);
		EXPECT_THROW((void)reg.create("quic"), std::out_of_range);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add a `HtmlDocument` + `HtmlDocumentCreator` without changing
 *    `DocumentCreator::save_stub` — only new types and wiring.
 * 2. Replace string keys in `TransportRegistry` with `std::type_index` or an
 *    `enum class` and a `switch` (compile-time exhaustiveness checks).
 * 3. Read about `virtual std::unique_ptr<Base> clone() const` as a kind of
 *    instance factory for prototypes — related to Factory Method + Prototype.
 */
