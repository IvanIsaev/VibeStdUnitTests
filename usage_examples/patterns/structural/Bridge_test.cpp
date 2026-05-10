/*
 * =============================================================================
 * Bridge (Gang of Four — Structural)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * **Decouple an abstraction** from its **implementation** so that the two can
 * vary **independently**.
 *
 * The abstraction (high-level types your clients name) holds a pointer or
 * reference to an **implementor** interface. Concrete abstractions (`Dialog`,
 * `Circle`) call into concrete implementors (`LinuxRenderer`, `PdfExporter`)
 * without inheriting from them — composition replaces a brittle inheritance
 * lattice (`LinuxPdfDialog`, `MacSvgDialog`, …).
 *
 * Typical structure
 * -----------------
 *   • **Abstraction** — defines the surface (`Window::draw()`), **delegates** to
 *     Implementor, may keep reference-counted or unique ownership.
 *   • **RefinedAbstraction** — optional subclasses that extend the abstraction
 *     (`ModalWindow`, `PopupWindow`) while reusing the same implementor port.
 *   • **Implementor** — stable interface for “how” (`IRenderTarget::blit`).
 *   • **ConcreteImplementor** — real behavior (`OpenGLTarget`, `CairoTarget`).
 *
 * Why use it
 * ----------
 *   • **Two orthogonal axes** — UI platform × document format, OS × driver, or
 *     policy × strategy at a coarser grain than tiny function objects.
 *
 *   • **Binary sizing** — avoid an M×N class matrix; you need M abstractions + N
 *     implementations plus wiring, not M×N combined types.
 *
 *   • **Runtime swap** — replace the implementor on an existing abstraction when
 *     settings change (theme, output device) without rebuilding the object graph.
 *
 * Bridge vs related ideas
 * -----------------------
 *   • **Adapter** fixes **mismatched** interfaces after the fact. **Bridge**
 *     designs **two parallel hierarchies** up front so they evolve on purpose.
 *
 *   • **Strategy** often swaps a **small algorithm** inside one context. **Bridge**
 *     is the larger **structural** split between “what the client names” and
 *     “how the platform does it,” sometimes spanning many methods.
 *
 *   • **PIMPL** hides **private members**; **Bridge** publishes an **abstract
 *     implementor** type meant to be **subclassed** in multiple modules.
 *
 * C++ implementation notes
 * ------------------------
 *   • Prefer **`std::unique_ptr<Implementor>`** owned by the Abstraction when
 *     lifetime matches; use **`shared_ptr`** when several abstractions share one
 *     heavy backend (GPU context).
 *
 *   • A **virtual destructor** on `Implementor` is required when deleting through
 *     the base pointer.
 *
 *   • **Inject** implementors from factories or DI containers so tests can supply
 *     fakes without `#ifdef TEST`.
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   class Report {
 *    public:
 *     explicit Report(std::unique_ptr<IExporter> ex) : exporter_(std::move(ex)) {}
 *     std::string publish(const Data& d) { return exporter_->export_report(d); }
 *    private:
 *     std::unique_ptr<IExporter> exporter_;
 *   };
 *   // Refined: MonthlyReport, ExecutiveSummary — same IExporter port.
 *
 * Pitfalls
 * --------
 *   • **Leaky abstraction** — if the Abstraction exposes raw implementor types,
 *     clients couple to both sides and the Bridge collapses.
 *
 *   • **Fat implementor** — one “god” backend interface forces unrelated systems
 *     into one file; split ports (`IInput`, `IOutput`) when cohesion drops.
 *
 *   • **Ownership bugs** — document who deletes the implementor when using raw
 *     pointers across DLL boundaries.
 *
 * Testing
 * -------
 *   • Unit-test each **ConcreteImplementor** with no UI.
 *   • Test each **RefinedAbstraction** against a **fake implementor** that
 *     records calls or returns canned strings.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

namespace usage_examples::patterns::structural {

	// -----------------------------------------------------------------
	// Implementor — how visual chrome is produced (theme / toolkit side).
	// -----------------------------------------------------------------
	struct ThemePainter
	{
		virtual ~ThemePainter() = default;
		[[nodiscard]] virtual std::string chrome(const std::string& label) const = 0;
	};

	struct LightThemePainter final : ThemePainter
	{
		[[nodiscard]] std::string chrome(const std::string& label) const override
		{
			return "LIGHT[" + label + "]";
		}
	};

	struct DarkThemePainter final : ThemePainter
	{
		[[nodiscard]] std::string chrome(const std::string& label) const override
		{
			return "DARK{" + label + "}";
		}
	};

	// -----------------------------------------------------------------
	// Abstraction — widget knows *what* to show; painter knows *how* it looks.
	// -----------------------------------------------------------------
	class Widget
	{
	public:
		explicit Widget(std::unique_ptr<ThemePainter> painter) : painter_(std::move(painter)) {}

		Widget(const Widget&) = delete;
		Widget& operator=(const Widget&) = delete;
		Widget(Widget&&) noexcept = default;
		Widget& operator=(Widget&&) noexcept = default;

		virtual ~Widget() = default;

		[[nodiscard]] virtual std::string render() const = 0;

	protected:
		[[nodiscard]] const ThemePainter& painter() const { return *painter_; }

	private:
		std::unique_ptr<ThemePainter> painter_;
	};

	class BannerWidget final : public Widget
	{
	public:
		BannerWidget(std::unique_ptr<ThemePainter> painter, std::string headline)
			: Widget(std::move(painter)), headline_(std::move(headline))
		{}

		[[nodiscard]] std::string render() const override { return painter().chrome(headline_); }

	private:
		std::string headline_;
	};

	class BadgeWidget final : public Widget
	{
	public:
		BadgeWidget(std::unique_ptr<ThemePainter> painter, std::string code)
			: Widget(std::move(painter)), code_(std::move(code))
		{}

		[[nodiscard]] std::string render() const override { return painter().chrome("badge:" + code_); }

	private:
		std::string code_;
	};

	// -----------------------------------------------------------------
	// Second bridge — audio backend vs alarm kind (orthogonal dimensions).
	// -----------------------------------------------------------------
	struct IBeepBackend
	{
		virtual ~IBeepBackend() = default;
		[[nodiscard]] virtual std::string emit_ms(int duration_ms) const = 0;
	};

	struct SoftBeepBackend final : IBeepBackend
	{
		[[nodiscard]] std::string emit_ms(int duration_ms) const override
		{
			return "soft-" + std::to_string(duration_ms) + "ms";
		}
	};

	struct LoudBeepBackend final : IBeepBackend
	{
		[[nodiscard]] std::string emit_ms(int duration_ms) const override
		{
			return "LOUD-" + std::to_string(duration_ms) + "ms";
		}
	};

	class Alarm
	{
	public:
		explicit Alarm(std::unique_ptr<IBeepBackend> backend) : backend_(std::move(backend)) {}

		Alarm(const Alarm&) = delete;
		Alarm& operator=(const Alarm&) = delete;
		Alarm(Alarm&&) noexcept = default;
		Alarm& operator=(Alarm&&) noexcept = default;

		virtual ~Alarm() = default;

		[[nodiscard]] virtual std::string ring() const = 0;

	protected:
		[[nodiscard]] const IBeepBackend& backend() const { return *backend_; }

	private:
		std::unique_ptr<IBeepBackend> backend_;
	};

	class KitchenTimerAlarm final : public Alarm
	{
	public:
		using Alarm::Alarm;

		[[nodiscard]] std::string ring() const override { return backend().emit_ms(150); }
	};

	class WakeUpAlarm final : public Alarm
	{
	public:
		using Alarm::Alarm;

		[[nodiscard]] std::string ring() const override { return backend().emit_ms(800); }
	};

} // namespace usage_examples::patterns::structural

namespace {

	using usage_examples::patterns::structural::BadgeWidget;
	using usage_examples::patterns::structural::BannerWidget;
	using usage_examples::patterns::structural::DarkThemePainter;
	using usage_examples::patterns::structural::KitchenTimerAlarm;
	using usage_examples::patterns::structural::LightThemePainter;
	using usage_examples::patterns::structural::LoudBeepBackend;
	using usage_examples::patterns::structural::SoftBeepBackend;
	using usage_examples::patterns::structural::WakeUpAlarm;

	TEST(BridgeUsageExamples, SameBannerRefinementSwapsThemeImplementation)
	{
		const BannerWidget light(std::make_unique<LightThemePainter>(), "Sale");
		const BannerWidget dark(std::make_unique<DarkThemePainter>(), "Sale");

		EXPECT_EQ(light.render(), "LIGHT[Sale]");
		EXPECT_EQ(dark.render(), "DARK{Sale}");
	}

	TEST(BridgeUsageExamples, DifferentRefinementsShareOnePainterConcept)
	{
		const BadgeWidget badge(std::make_unique<DarkThemePainter>(), "42");
		EXPECT_EQ(badge.render(), "DARK{badge:42}");
	}

	TEST(BridgeUsageExamples, AlarmRefinementsUseDifferentDurationsSameBackend)
	{
		const KitchenTimerAlarm kitchen(std::make_unique<SoftBeepBackend>());
		const WakeUpAlarm wake(std::make_unique<SoftBeepBackend>());

		EXPECT_EQ(kitchen.ring(), "soft-150ms");
		EXPECT_EQ(wake.ring(), "soft-800ms");
	}

	TEST(BridgeUsageExamples, SameAlarmKindSwapsBeepImplementation)
	{
		const KitchenTimerAlarm quiet(std::make_unique<SoftBeepBackend>());
		const KitchenTimerAlarm noisy(std::make_unique<LoudBeepBackend>());

		EXPECT_EQ(quiet.ring(), "soft-150ms");
		EXPECT_EQ(noisy.ring(), "LOUD-150ms");
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Extract `IThemePainter` into its own header and provide a mock for widget
 *    tests that records `chrome()` arguments.
 * 2. Allow runtime `set_painter(std::unique_ptr<ThemePainter>)` on a live
 *    `BannerWidget` and assert `render()` reflects the swap.
 * 3. Contrast with **Abstract Factory**: a factory could return matched pairs
 *    (widget + theme) — sketch when Factory + Bridge compose vs when either alone
 *    suffices.
 */
