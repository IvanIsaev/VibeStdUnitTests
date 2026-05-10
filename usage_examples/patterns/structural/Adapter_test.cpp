/*
 * =============================================================================
 * Adapter (Gang of Four — Structural)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Convert the interface of a class into another interface clients expect.
 * Adapter lets classes work together that could not otherwise because of
 * incompatible interfaces.
 *
 * Think **power plug adapters**, **USB-C to HDMI**, or wrapping a third-party
 * SDK so the rest of your code speaks your domain types only.
 *
 * Participants (typical)
 * ----------------------
 *   • **Target** — the interface the client already uses (`Renderer`, `PaymentPort`).
 *   • **Adaptee** — the existing type with the “wrong” shape (`LegacyGpu`, `PayPalSdk`).
 *   • **Adapter** — implements Target and translates calls into Adaptee operations.
 *   • **Client** — depends only on Target.
 *
 * Object adapter vs class adapter
 * -------------------------------
 *   • **Object adapter (composition)** — the Adapter *holds* a reference or
 *     pointer to an Adaptee instance. One adapter can wrap many adaptees, you
 *     can swap implementations, and you avoid multiple inheritance.
 *
 *   • **Class adapter (inheritance)** — the Adapter *publicly inherits* Target
 *     and (often) Adaptee, forwarding methods. Tighter coupling; only viable if
 *     Adaptee is a class you may subclass and its lifetime matches the adapter.
 *
 * In modern C++, **composition** is usually the default; MI-based class adapters
 * appear in legacy interop or when wrapping C-style base structs.
 *
 * Adapter vs related patterns
 * ---------------------------
 *   • **Facade** simplifies a *subsystem*; Adapter maps *one* interface to
 *     another (often 1:1 translation).
 *
 *   • **Bridge** separates abstraction from implementation with stable interfaces
 *     on both sides; Adapter connects *existing* code you did not design together.
 *
 *   • **Decorator** adds responsibilities while keeping the same interface;
 *     Adapter changes the interface to match a different contract.
 *
 * When to use
 * -----------
 *   • Integrating **third-party** or **legacy** libraries without rewriting callers.
 *   • **Testing** — adapt production types to narrow test doubles or vice versa.
 *   • **Versioning** — expose v2 API while internally calling v1 implementation.
 *
 * C++ implementation notes
 * ------------------------
 *   • Prefer **object adapters** taking `Adaptee&` or `std::shared_ptr<Adaptee>`
 *     so lifetime is explicit.
 *
 *   • **Small translations** can be a **lambda** or `std::function` passed where
 *     a Target expects a callback — a functional adapter.
 *
 *   • If Target is a large interface, consider an **explicit port** type in your
 *     domain and only adapt methods you truly need (Interface Segregation).
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   // Object adapter: wrap a C API
 *   class FileSinkAdapter : public IStreamSink {
 *    public:
 *     explicit FileSinkAdapter(FILE* f) : file_(f) {}
 *     void write(std::string_view s) override { fwrite(s.data(), 1, s.size(), file_); }
 *    private:
 *     FILE* file_;
 *   };
 *
 *   // Functional adapter at a boundary:
 *   auto to_int_parser = [](std::string_view sv) { return std::stoi(std::string{sv}); };
 *
 * Pitfalls
 * --------
 *   • **Lifetime** — Adapter must not outlive the Adaptee it references unless
 *     ownership is shared clearly.
 *
 *   • **Re-entrancy** — if Adaptee calls back into code that uses the Adapter,
 *     document ordering and locks.
 *
 *   • **Leaky translation** — exceptions, error codes, and thread affinity differ
 *     across APIs; map them deliberately in one place (the Adapter).
 *
 * Testing
 * -------
 *   • Unit-test the Adapter against a **fake Adaptee** that records arguments.
 *   • Contract-test that Target pre/postconditions match what Adaptee requires.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <string>

namespace usage_examples::patterns::structural {

	// -----------------------------------------------------------------
	// Target — what new code expects (abstract port).
	// -----------------------------------------------------------------
	struct IRectangleRaster
	{
		virtual ~IRectangleRaster() = default;

		// Top-left (x, y) plus width and height in pixels.
		virtual void fill_rectangle(int x, int y, int width, int height) = 0;
	};

	// -----------------------------------------------------------------
	// Adaptee — legacy API using inclusive corner coordinates.
	// Cannot change (vendor library, old module).
	// -----------------------------------------------------------------
	struct LegacyBlitter
	{
		int last_left = 0;
		int last_top = 0;
		int last_right = 0;
		int last_bottom = 0;

		void blit_inclusive_rect(int left, int top, int right, int bottom)
		{
			last_left = left;
			last_top = top;
			last_right = right;
			last_bottom = bottom;
		}
	};

	// -----------------------------------------------------------------
	// Object adapter — composition over LegacyBlitter.
	// -----------------------------------------------------------------
	class BlitterRectangleAdapter final : public IRectangleRaster
	{
	public:
		explicit BlitterRectangleAdapter(LegacyBlitter& blitter) : blitter_(blitter) {}

		void fill_rectangle(int x, int y, int width, int height) override
		{
			blitter_.blit_inclusive_rect(x, y, x + width, y + height);
		}

	private:
		LegacyBlitter& blitter_;
	};

	// -----------------------------------------------------------------
	// Target for streaming text — small second scenario.
	// -----------------------------------------------------------------
	struct ILineWriter
	{
		virtual ~ILineWriter() = default;
		virtual void write_line(const std::string& line) = 0;
	};

	// Adaptee: only accepts C string + implicit newline elsewhere in the app.
	struct CStringLogger
	{
		std::string last_message;

		void append_c_str(const char* msg) { last_message = msg ? msg : ""; }
	};

	class LoggerLineAdapter final : public ILineWriter
	{
	public:
		explicit LoggerLineAdapter(CStringLogger& sink) : sink_(sink) {}

		void write_line(const std::string& line) override { sink_.append_c_str(line.c_str()); }

	private:
		CStringLogger& sink_;
	};

	// -----------------------------------------------------------------
	// Class adapter — multiple inheritance: implement Target by subclassing
	// a concrete Adaptee (use sparingly; Adaptee must tolerate derivation).
	// -----------------------------------------------------------------
	struct ILegacyCounterSink
	{
		virtual ~ILegacyCounterSink() = default;
		virtual void put_value(int v) = 0;
	};

	struct DecimalEmitter
	{
		int last_value = 0;

		void emit_decimal(int v) { last_value = v; }
	};

	struct CounterSinkClassAdapter final : ILegacyCounterSink, DecimalEmitter
	{
		void put_value(int v) override { emit_decimal(v); }
	};

	// Client code that only knows Target.
	inline void paint_tile(IRectangleRaster& target, int col, int row, int cell)
	{
		const int x = col * cell;
		const int y = row * cell;
		target.fill_rectangle(x, y, cell, cell);
	}

} // namespace usage_examples::patterns::structural

namespace {

	using usage_examples::patterns::structural::BlitterRectangleAdapter;
	using usage_examples::patterns::structural::CounterSinkClassAdapter;
	using usage_examples::patterns::structural::CStringLogger;
	using usage_examples::patterns::structural::LegacyBlitter;
	using usage_examples::patterns::structural::LoggerLineAdapter;
	using usage_examples::patterns::structural::paint_tile;

	TEST(AdapterUsageExamples, ObjectAdapterTranslatesWidthHeightToInclusiveCorners)
	{
		LegacyBlitter legacy;
		BlitterRectangleAdapter adapter(legacy);
		adapter.fill_rectangle(10, 20, 30, 40);
		EXPECT_EQ(legacy.last_left, 10);
		EXPECT_EQ(legacy.last_top, 20);
		EXPECT_EQ(legacy.last_right, 40);
		EXPECT_EQ(legacy.last_bottom, 60);
	}

	TEST(AdapterUsageExamples, ClientUsesTargetThroughAdapterOnly)
	{
		LegacyBlitter legacy;
		BlitterRectangleAdapter adapter(legacy);
		paint_tile(adapter, 2, 3, 16);
		EXPECT_EQ(legacy.last_left, 32);
		EXPECT_EQ(legacy.last_top, 48);
		EXPECT_EQ(legacy.last_right, 48);
		EXPECT_EQ(legacy.last_bottom, 64);
	}

	TEST(AdapterUsageExamples, LoggerObjectAdapterPassesCStrToAdaptee)
	{
		CStringLogger sink;
		LoggerLineAdapter adapter(sink);
		adapter.write_line("ping");
		EXPECT_EQ(sink.last_message, "ping");
	}

	TEST(AdapterUsageExamples, ClassAdapterForwardsPutValueToEmitDecimal)
	{
		CounterSinkClassAdapter adapter;
		adapter.put_value(42);
		EXPECT_EQ(adapter.last_value, 42);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add an Adapter that maps `std::expected<void,E>` from a modern API into
 *    legacy `bool` plus `GetLastError()`-style side channel.
 * 2. Wrap a blocking Adaptee in an Adapter that exposes `async`/`future` to
 *    match a Target used by coroutine-based callers.
 * 3. Sketch a **two-way** adapter between DTOs in two microservices (field rename
 *    and unit conversion in one struct).
 */
