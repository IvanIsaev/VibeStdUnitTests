/*
 * =============================================================================
 * Abstract Factory (Gang of Four — Creational)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Provide an interface for creating *families* of related or dependent objects
 * without specifying their concrete classes.
 *
 * A single Abstract Factory groups several factory methods at once. Each
 * Concrete Factory implements *all* of them so the products stay compatible:
 * a "Light" factory yields light-styled widgets; a "Dark" factory yields
 * dark-styled widgets. The client depends only on abstract product interfaces
 * and the abstract factory — not on `LightButton` vs `DarkButton` names.
 *
 * Typical structure
 * -----------------
 *   • AbstractFactory   — declares `createProductA()`, `createProductB()`, …
 *   • ConcreteFactoryX  — each method returns implementations from family X
 *   • AbstractProductA, AbstractProductB — interfaces for parallel hierarchies
 *   • ConcreteProductA1 + ConcreteProductB1 — family 1 (consistent together)
 *   • ConcreteProductA2 + ConcreteProductB2 — family 2
 *
 * Collaboration
 * -------------
 * Usually a client receives `AbstractFactory&` (or `unique_ptr`) and calls
 * several `create_*` methods to assemble a feature (dialog, level, theme).
 * Because every product comes from the *same* concrete factory instance, you
 * preserve cross-product invariants (shared palette, behavior assumptions).
 *
 * Abstract Factory vs Factory Method
 * -----------------------------------
 *   • Factory Method: one product type per Creator subclass; one virtual hook.
 *   • Abstract Factory: *multiple* product types per factory; often implemented
 *     as a cluster of Factory Methods on one Abstract Factory interface.
 *
 * In other words, Abstract Factory is often "several factory methods behind
 * one polymorphic type," while Factory Method focuses on subclassing one hook.
 *
 * Abstract Factory vs Builder
 * ----------------------------
 *   • Abstract Factory emphasizes *families* of products (parallel types).
 *   • Builder emphasizes *step-by-step* construction of *one* complex object
 *     with many optional parts. They can be combined (builder uses a factory
 *     for parts).
 *
 * When to use
 * -----------
 *   • You have interchangeable product families (themes, platforms, brands).
 *   • You must enforce that products from family A are never mixed with B
 *     through the same construction path (compile-time or runtime factory).
 *   • You want to hide concrete class names from high-level modules.
 *
 * When to reconsider
 * ------------------
 *   • Adding a *new kind of product* (new abstract interface) touches every
 *     concrete factory — the pattern trades "easy new family" for "hard new
 *     product type" unless you refactor (visitor-style registries, generics).
 *   • Very small systems: a few `if (theme == dark)` branches may be simpler.
 *
 * C++ implementation notes
 * ------------------------
 *   • Return `std::unique_ptr<AbstractProduct>` from each `create_*` method.
 *     Give every abstract product a `virtual` destructor.
 *
 *   • If factories are stateless, they can be constexpr-friendly singletons or
 *     stack objects passed by const reference.
 *
 *   • For testability, inject `AbstractFactory` into clients instead of using
 *     global `get_factory()`.
 *
 *   • C++20 concepts can document "anything that can build Button+Checkbox,"
 *     but the classic pattern remains runtime-polymorphic factories.
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   // Abstract products (parallel hierarchies):
 *   struct Button { virtual ~Button() = default; virtual void paint() = 0; };
 *   struct Checkbox { virtual ~Checkbox() = default; virtual void paint() = 0; };
 *
 *   // Abstract factory — one entry point per product *role*:
 *   struct WidgetFactory {
 *     virtual ~WidgetFactory() = default;
 *     virtual std::unique_ptr<Button> create_button() = 0;
 *     virtual std::unique_ptr<Checkbox> create_checkbox() = 0;
 *   };
 *
 *   // Concrete factory — entire family:
 *   struct MacWidgetFactory : WidgetFactory {
 *     std::unique_ptr<Button> create_button() override {
 *       return std::make_unique<MacButton>();
 *     }
 *     std::unique_ptr<Checkbox> create_checkbox() override {
 *       return std::make_unique<MacCheckbox>();
 *     }
 *   };
 *
 *   // Client code depends only on abstract types:
 *   void build_dialog(WidgetFactory& f) {
 *     auto ok = f.create_button();
 *     auto remember = f.create_checkbox();
 *     // both are stylistically consistent — same family
 *   }
 *
 * Variations and modern twists
 * ----------------------------
 *   • **Functional style:** pass a struct of lambdas `(make_button, make_checkbox)`
 *     instead of a class — same idea, less inheritance.
 *
 *   • **Abstract factory + prototype:** factories clone preconfigured prototypes
 *     instead of calling `make_unique` with literals.
 *
 *   • **Registries:** map `ThemeId` → factory function returning
 *     `unique_ptr<WidgetFactory>` for plugin-loaded themes.
 *
 * Pitfalls
 * --------
 *   • **Product explosion:** N families × M product roles = many classes.
 *     Document the matrix; consider modules or code generation for huge grids.
 *
 *   • **Non-virtual destructor** on any abstract base → UB when deleting via
 *     base pointer.
 *
 *   • **Mixing families** by accident if client code calls two different
 *     concrete factories for one dialog — establish one factory per scope.
 *
 * Testing
 * -------
 * Supply a test double factory that returns lightweight fakes or captures
 * calls, so client logic is tested without real OS widgets or assets.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

namespace usage_examples::patterns::behavioral {

	// -----------------------------------------------------------------
	// Abstract products — two parallel hierarchies (widget "roles").
	// -----------------------------------------------------------------
	struct Button
	{
		virtual ~Button() = default;
		[[nodiscard]] virtual std::string label_style() const = 0;
	};

	struct Checkbox
	{
		virtual ~Checkbox() = default;
		[[nodiscard]] virtual std::string tick_style() const = 0;
	};

	// Family: "light" UI
	struct LightButton final : Button
	{
		[[nodiscard]] std::string label_style() const override { return "light-flat"; }
	};

	struct LightCheckbox final : Checkbox
	{
		[[nodiscard]] std::string tick_style() const override { return "light-hollow"; }
	};

	// Family: "dark" UI
	struct DarkButton final : Button
	{
		[[nodiscard]] std::string label_style() const override { return "dark-raised"; }
	};

	struct DarkCheckbox final : Checkbox
	{
		[[nodiscard]] std::string tick_style() const override { return "dark-filled"; }
	};

	// -----------------------------------------------------------------
	// Abstract Factory — creates one matched set of products.
	// -----------------------------------------------------------------
	struct WidgetFamilyFactory
	{
		virtual ~WidgetFamilyFactory() = default;

		[[nodiscard]] virtual std::unique_ptr<Button> create_button() const = 0;
		[[nodiscard]] virtual std::unique_ptr<Checkbox> create_checkbox() const = 0;
	};

	struct LightWidgetFamilyFactory final : WidgetFamilyFactory
	{
		[[nodiscard]] std::unique_ptr<Button> create_button() const override
		{
			return std::make_unique<LightButton>();
		}

		[[nodiscard]] std::unique_ptr<Checkbox> create_checkbox() const override
		{
			return std::make_unique<LightCheckbox>();
		}
	};

	struct DarkWidgetFamilyFactory final : WidgetFamilyFactory
	{
		[[nodiscard]] std::unique_ptr<Button> create_button() const override
		{
			return std::make_unique<DarkButton>();
		}

		[[nodiscard]] std::unique_ptr<Checkbox> create_checkbox() const override
		{
			return std::make_unique<DarkCheckbox>();
		}
	};

	// Client: uses only abstract factory + abstract products.
	struct DialogParts
	{
		std::string button_style;
		std::string checkbox_style;
	};

	inline DialogParts make_sign_in_dialog(const WidgetFamilyFactory& factory)
	{
		auto button = factory.create_button();
		auto checkbox = factory.create_checkbox();
		return DialogParts{ button->label_style(), checkbox->tick_style() };
	}

	// -----------------------------------------------------------------
	// Second miniature family — media codecs (another "parallel" pair).
	// Illustrates the same pattern with different domain names.
	// -----------------------------------------------------------------
	struct AudioDecoder
	{
		virtual ~AudioDecoder() = default;
		[[nodiscard]] virtual std::string format_tag() const = 0;
	};

	struct VideoDecoder
	{
		virtual ~VideoDecoder() = default;
		[[nodiscard]] virtual std::string format_tag() const = 0;
	};

	struct OpenSourceFlacDecoder final : AudioDecoder
	{
		[[nodiscard]] std::string format_tag() const override { return "flac-lib"; }
	};

	struct OpenSourceVp9Decoder final : VideoDecoder
	{
		[[nodiscard]] std::string format_tag() const override { return "vp9-lib"; }
	};

	struct ProprietaryAacDecoder final : AudioDecoder
	{
		[[nodiscard]] std::string format_tag() const override { return "aac-sdk"; }
	};

	struct ProprietaryHevcDecoder final : VideoDecoder
	{
		[[nodiscard]] std::string format_tag() const override { return "hevc-sdk"; }
	};

	struct CodecKitFactory
	{
		virtual ~CodecKitFactory() = default;
		[[nodiscard]] virtual std::unique_ptr<AudioDecoder> create_audio_decoder() const = 0;
		[[nodiscard]] virtual std::unique_ptr<VideoDecoder> create_video_decoder() const = 0;
	};

	struct OpenSourceCodecKit final : CodecKitFactory
	{
		[[nodiscard]] std::unique_ptr<AudioDecoder> create_audio_decoder() const override
		{
			return std::make_unique<OpenSourceFlacDecoder>();
		}

		[[nodiscard]] std::unique_ptr<VideoDecoder> create_video_decoder() const override
		{
			return std::make_unique<OpenSourceVp9Decoder>();
		}
	};

	struct ProprietaryCodecKit final : CodecKitFactory
	{
		[[nodiscard]] std::unique_ptr<AudioDecoder> create_audio_decoder() const override
		{
			return std::make_unique<ProprietaryAacDecoder>();
		}

		[[nodiscard]] std::unique_ptr<VideoDecoder> create_video_decoder() const override
		{
			return std::make_unique<ProprietaryHevcDecoder>();
		}
	};

	inline std::pair<std::string, std::string> pipeline_tags(const CodecKitFactory& kit)
	{
		auto audio = kit.create_audio_decoder();
		auto video = kit.create_video_decoder();
		return { audio->format_tag(), video->format_tag() };
	}

} // namespace usage_examples::patterns::behavioral

namespace {

	using usage_examples::patterns::behavioral::DarkWidgetFamilyFactory;
	using usage_examples::patterns::behavioral::LightWidgetFamilyFactory;
	using usage_examples::patterns::behavioral::make_sign_in_dialog;
	using usage_examples::patterns::behavioral::OpenSourceCodecKit;
	using usage_examples::patterns::behavioral::pipeline_tags;
	using usage_examples::patterns::behavioral::ProprietaryCodecKit;

	TEST(AbstractFactoryUsageExamples, LightFamilyProducesMatchingWidgetStyles)
	{
		const LightWidgetFamilyFactory factory;
		const auto parts = make_sign_in_dialog(factory);
		EXPECT_EQ(parts.button_style, "light-flat");
		EXPECT_EQ(parts.checkbox_style, "light-hollow");
	}

	TEST(AbstractFactoryUsageExamples, DarkFamilyProducesDifferentMatchingPair)
	{
		const DarkWidgetFamilyFactory factory;
		const auto parts = make_sign_in_dialog(factory);
		EXPECT_EQ(parts.button_style, "dark-raised");
		EXPECT_EQ(parts.checkbox_style, "dark-filled");
	}

	TEST(AbstractFactoryUsageExamples, OpenSourceCodecKitStaysInOneFamily)
	{
		const OpenSourceCodecKit kit;
		const auto [a, v] = pipeline_tags(kit);
		EXPECT_EQ(a, "flac-lib");
		EXPECT_EQ(v, "vp9-lib");
	}

	TEST(AbstractFactoryUsageExamples, ProprietaryCodecKitUsesAlternateDecoders)
	{
		const ProprietaryCodecKit kit;
		const auto [a, v] = pipeline_tags(kit);
		EXPECT_EQ(a, "aac-sdk");
		EXPECT_EQ(v, "hevc-sdk");
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add `create_label()` to `WidgetFamilyFactory` and implement it in both
 *    light and dark factories — notice every concrete factory must change.
 * 2. Implement a `RecordingWidgetFactory` test double that counts how many
 *    times each `create_*` was called.
 * 3. Sketch a `std::unique_ptr<WidgetFamilyFactory> make_factory(ThemeId id)`
 *    that returns `Light` or `Dark` without the client using `switch` on
 *    widget types — only on the factory choice.
 */
