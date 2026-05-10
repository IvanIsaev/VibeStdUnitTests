/*
 * =============================================================================
 * Facade (Gang of Four — Structural)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Provide a **unified interface** to a set of interfaces in a subsystem. Facade
 * defines a **higher-level** interface that makes the subsystem **easier to use**.
 *
 * Clients talk to **one** entry type; behind it, several collaborating objects
 * run the real protocol (ordering, validation, retries, logging). The subsystem
 * can remain richly factored without forcing every caller to learn its choreography.
 *
 * Typical structure
 * -----------------
 *   • **Facade** — thin coordinator with methods like `start()`, `submitOrder()`.
 *   • **Subsystem classes** — many small services the Facade orchestrates; they
 *     may know nothing about the Facade.
 *   • **Client** — depends on the Facade (or an interface implemented by it),
 *     not on every subsystem type.
 *
 * Why use it
 * ----------
 *   • **Reduce coupling** — UI, scripts, or remote handlers call one port instead
 *     of N constructors and call-order rules.
 *
 *   • **Document workflows** — the Facade method *is* the recipe: reserve stock,
 *     charge card, schedule shipping, notify user.
 *
 *   • **Evolve internals** — swap a payment provider or split inventory into
 *     microservices while keeping the Facade signature stable for callers.
 *
 * Facade vs related patterns
 * --------------------------
 *   • **Adapter** bridges **two** specific interfaces (old SDK vs your port).
 *     **Facade** simplifies **many** subsystem types behind **one** face — not
 *     necessarily adapting foreign types.
 *
 *   • **Mediator** centralizes **peer-to-peer** communication; **Facade** is
 *     usually **one-way** orchestration from client into the subsystem.
 *
 *   • **Bridge** separates abstraction from implementation with parallel
 *     hierarchies; **Facade** is a single orchestration layer, not necessarily
 *     a parallel hierarchy.
 *
 * What a Facade is *not*
 * ----------------------
 *   • Not a mandate to hide **bad** subsystem design — long term, fix awkward
 *     collaborators rather than only wrapping them.
 *
 *   • Not always a **God object** — keep Facade methods cohesive; split facades
 *     by bounded context (`BillingFacade`, `FulfillmentFacade`) when a single
 *     type grows huge.
 *
 * C++ implementation notes
 * ------------------------
 *   • Hold subsystem objects by **reference** or **`shared_ptr`** when lifetime
 *     is shared; construct subsystems in `main` / composition root and inject.
 *
 *   • Consider an **interface** (`ICheckoutService`) implemented by the Facade
 *     for testing and mocking.
 *
 *   • **Transactions:** if step 3 fails after step 1 succeeded, document and
 *     implement **compensation** (restock, void charge) inside the Facade.
 *
 * Commented micro-examples (mental model)
 * ---------------------------------------
 *
 *   class CompilerFacade {
 *    public:
 *     bool compile_file(const std::filesystem::path& src) {
 *       lexer_.reset();
 *       parser_.parse(lexer_.tokenize(read(src)));
 *       codegen_.emit(parser_.ast());
 *       return !diag_.has_errors();
 *     }
 *    private:
 *     Lexer lexer_; Parser parser_; CodeGen codegen_; Diagnostics diag_;
 *   };
 *
 * Pitfalls
 * --------
 *   • **Over-wide API** — one Facade type with dozens of unrelated methods
 *     becomes a maintenance bottleneck; split or use modules.
 *
 *   • **Leaky failures** — map subsystem exceptions to stable error types or
 *     `std::expected` at the Facade boundary.
 *
 *   • **Hidden temporal coupling** — document thread-safety and which methods
 *     must be called before others if you expose more than one entry point.
 *
 * Testing
 * -------
 *   • **Integration-test** the Facade against real or in-memory subsystems.
 *   • **Unit-test** subsystems independently; use fakes when isolating Facade
 *     branching (out-of-stock vs declined payment).
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <string>

namespace usage_examples::patterns::structural {

	// -----------------------------------------------------------------
	// Subsystem types — intentionally small and chatty (many calls to get work
	// done); the Facade sequences them for the client.
	// -----------------------------------------------------------------
	class InventorySubsystem
	{
	public:
		explicit InventorySubsystem(int units_in_stock) : stock_(units_in_stock) {}

		[[nodiscard]] bool try_reserve(std::string_view /*sku*/, int quantity)
		{
			if (quantity < 0 || quantity > stock_) return false;
			stock_ -= quantity;
			last_reserved_ = quantity;
			return true;
		}

		void release_reservation(int quantity) { stock_ += quantity; }

		[[nodiscard]] int stock() const { return stock_; }

		[[nodiscard]] int last_reserved() const { return last_reserved_; }

	private:
		int stock_;
		int last_reserved_{ 0 };
	};

	class PaymentSubsystem
	{
	public:
		[[nodiscard]] bool authorize_total(int total_cents, std::string_view /*merchant_id*/)
		{
			last_amount_ = total_cents;
			const bool ok = total_cents > 0 && total_cents < 1'000'000;
			last_ok_ = ok;
			return ok;
		}

		[[nodiscard]] bool last_success() const { return last_ok_; }

		[[nodiscard]] int last_amount() const { return last_amount_; }

	private:
		bool last_ok_{ false };
		int last_amount_{ 0 };
	};

	class ShippingSubsystem
	{
	public:
		void schedule_pickup(std::string_view order_id)
		{
			last_tracking_ = std::string("TRK-");
			last_tracking_.append(order_id);
		}

		[[nodiscard]] const std::string& tracking_code() const { return last_tracking_; }

	private:
		std::string last_tracking_;
	};

	class NotificationSubsystem
	{
	public:
		void notify_email(std::string_view /*to*/, std::string_view message) { last_body_ = std::string(message); }

		[[nodiscard]] const std::string& last_message() const { return last_body_; }

	private:
		std::string last_body_;
	};

	// -----------------------------------------------------------------
	// Facade — one method expresses the multi-step checkout story.
	// -----------------------------------------------------------------
	class ShopCheckoutFacade
	{
	public:
		ShopCheckoutFacade(InventorySubsystem& inventory,
			PaymentSubsystem& payment,
			ShippingSubsystem& shipping,
			NotificationSubsystem& notifications)
			: inventory_(inventory), payment_(payment), shipping_(shipping), notifications_(notifications)
		{}

		// Returns false if inventory cannot satisfy or payment is declined.
		[[nodiscard]] bool place_order(std::string_view sku,
			int quantity,
			int unit_price_cents,
			std::string_view order_id,
			std::string_view customer_email)
		{
			if (!inventory_.try_reserve(sku, quantity)) return false;

			const int total_cents = quantity * unit_price_cents;
			if (!payment_.authorize_total(total_cents, "demo-merchant"))
			{
				inventory_.release_reservation(quantity);
				return false;
			}

			shipping_.schedule_pickup(order_id);
			notifications_.notify_email(customer_email, "Your order is on its way.");
			return true;
		}

	private:
		InventorySubsystem& inventory_;
		PaymentSubsystem& payment_;
		ShippingSubsystem& shipping_;
		NotificationSubsystem& notifications_;
	};

} // namespace usage_examples::patterns::structural

namespace {

	using usage_examples::patterns::structural::InventorySubsystem;
	using usage_examples::patterns::structural::NotificationSubsystem;
	using usage_examples::patterns::structural::PaymentSubsystem;
	using usage_examples::patterns::structural::ShippingSubsystem;
	using usage_examples::patterns::structural::ShopCheckoutFacade;

	TEST(FacadeUsageExamples, PlaceOrderRunsSubsystemWorkflowOnSuccess)
	{
		InventorySubsystem inventory(10);
		PaymentSubsystem payment;
		ShippingSubsystem shipping;
		NotificationSubsystem notifications;
		ShopCheckoutFacade checkout(inventory, payment, shipping, notifications);

		ASSERT_TRUE(checkout.place_order("SKU-1", 2, 500, "ORD-42", "pat@example.com"));

		EXPECT_EQ(inventory.stock(), 8);
		EXPECT_EQ(inventory.last_reserved(), 2);
		EXPECT_TRUE(payment.last_success());
		EXPECT_EQ(payment.last_amount(), 1000);
		EXPECT_EQ(shipping.tracking_code(), "TRK-ORD-42");
		EXPECT_EQ(notifications.last_message(), "Your order is on its way.");
	}

	TEST(FacadeUsageExamples, PlaceOrderFailsWhenOutOfStock)
	{
		InventorySubsystem inventory(1);
		PaymentSubsystem payment;
		ShippingSubsystem shipping;
		NotificationSubsystem notifications;
		ShopCheckoutFacade checkout(inventory, payment, shipping, notifications);

		EXPECT_FALSE(checkout.place_order("SKU-1", 5, 100, "ORD-9", "a@b.c"));

		EXPECT_EQ(inventory.stock(), 1);
		EXPECT_FALSE(payment.last_success());
		EXPECT_TRUE(shipping.tracking_code().empty());
		EXPECT_TRUE(notifications.last_message().empty());
	}

	TEST(FacadeUsageExamples, PlaceOrderRestocksWhenPaymentDeclines)
	{
		InventorySubsystem inventory(5);
		PaymentSubsystem payment;
		ShippingSubsystem shipping;
		NotificationSubsystem notifications;
		ShopCheckoutFacade checkout(inventory, payment, shipping, notifications);

		EXPECT_FALSE(checkout.place_order("SKU-1", 1, 0, "ORD-X", "a@b.c"));

		EXPECT_EQ(inventory.stock(), 5);
		EXPECT_FALSE(payment.last_success());
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Introduce `std::expected<void, CheckoutError>` at the Facade boundary with
 *    distinct errors for stock, payment, and shipping failures.
 * 2. Split `ShopCheckoutFacade` into `ReservationFacade` + `FulfillmentFacade` if
 *    the workflow grows branches (B2B vs B2C).
 * 3. Compare with a **script** or **saga** that runs the same steps with
 *    retries and timeouts in a message-driven system.
 */
