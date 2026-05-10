/*
 * =============================================================================
 * Chain of Responsibility (Gang of Four — Behavioral)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Avoid coupling the **sender** of a request to its **receiver** by giving more
 * than one object a chance to **handle** the request. Chain the receiving
 * objects and **pass the request along the chain** until an object handles it.
 *
 * Think of a helpdesk ticket, an exception stack, or HTTP middleware: each link
 * can **consume** the request (stop propagation) or **forward** it.
 *
 * Participants
 * ------------
 *   • **Handler** — defines common interface (`handle`); usually knows a
 *     **successor** link (or obtains the next handler from a list).
 *
 *   • **ConcreteHandler** — decides whether it can process the request; if not,
 *     forwards to the successor (if any).
 *
 *   • **Client** — initiates the request on the **first** handler in the chain.
 *     The client often **does not know** which concrete class will ultimately
 *     satisfy the request.
 *
 * Two common flavors
 * ------------------
 *   • **Exclusive handling** — the first handler that *can* serve the request
 *     stops the chain (classic GoF; approval limits, exception translation).
 *
 *   • **Filter / pipeline** — *every* handler may run (logging, metrics), and
 *     some may **abort** early (auth failure). This blends CoR with **interceptor**
 *     chains in web frameworks.
 *
 * Why use it
 * ----------
 *   • **Open/Closed** — add a new handler by **linking** it without editing the
 *     client’s dispatch `switch`.
 *
 *   • **Decoupling** — sender depends on the abstract handler surface, not on N
 *     concrete services.
 *
 *   • **Dynamic order** — reorder or insert handlers at runtime (feature flags,
 *     A/B middleware stacks).
 *
 * Trade-offs and pitfalls
 * -----------------------
 *   • **No guarantee of handling** — if the chain ends without a match, define
 *     whether that is **valid**, an **error**, or should hit a **default sink**
 *     handler.
 *
 *   • **Performance** — worst case walks the whole chain; for hot paths, prefer
 *     **direct routing** (hash map), **jump tables**, or **small fixed chains**.
 *
 *   • **Ordering bugs** — “auth after cache” vs “cache after auth” changes
 *     semantics; document **invariants** (e.g., rate limit before expensive work).
 *
 *   • **Cycles** — guard against accidental rings when handlers **mutate** the
 *     `next` pointer or re-enter `handle` recursively without a base case.
 *
 * Related patterns
 * ----------------
 *   • **Composite** — handlers can form trees (UI event bubbling) instead of a
 *     simple linear list.
 *
 *   • **Decorator** — wraps one object to add behavior; CoR typically **chooses
 *     one** responsible peer along a list (unless you model filters).
 *
 *   • **Observer** — **broadcasts** to many subscribers; CoR is a **linear
 *     delegation** until someone claims the request (in the exclusive variant).
 *
 *   • **Command** — encapsulate the request as an object; a command may be
 *     passed down a chain of invokers / undo stacks.
 *
 * C++ implementation notes
 * ------------------------
 *   • **Raw `Handler* next`** — simple, explicit; lifetime owned by the caller or
 *     a registry (arena, static storage).
 *
 *   • **`std::unique_ptr` links** — owning chain from head to tail; careful when
 *     reordering or sharing tails.
 *
 *   • **`std::function` / table of lambdas** — great for **small** policies;
 *     store in `std::vector` and iterate until one returns `true`.
 *
 *   • **Coroutines / async** — each stage `co_await`s I/O; still decide whether
 *     later stages run after a terminal response.
 *
 * Testing
 * -------
 *   • **Matrix** — for each category of input, assert **which** handler fires and
 *     that earlier links are skipped appropriately.
 *
 *   • **Fall-through** — verify the **terminal** handler runs when no specialist
 *     matches.
 *
 *   • **Regression on order** — reorder links in a test and assert behavior
 *     changes exactly as specified.
 *
 *   • **One-definition rule** — types in this demo live in
 *     `chain_of_responsibility_gof` so they do not collide with other
 *     `usage_examples` sources.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <vector>

namespace usage_examples::patterns::behavioral::chain_of_responsibility_gof {

	// -----------------------------------------------------------------
	// Example 1 — Spending approval: each role owns a ceiling; forward upward.
	// Amounts are in **cents** to avoid floating point in examples.
	// -----------------------------------------------------------------
	struct SpendRequest
	{
		int amount_cents{};
		std::string approved_by_role;
	};

	class SpendApproverLink
	{
	public:
		virtual ~SpendApproverLink() = default;

		void set_next(SpendApproverLink* successor) noexcept { next_ = successor; }

		void handle(SpendRequest& req)
		{
			if (try_approve(req)) return;
			if (next_) next_->handle(req);
		}

	protected:
		virtual bool try_approve(SpendRequest& req) = 0;

	private:
		SpendApproverLink* next_{};
	};

	class JuniorSpendApprover final : public SpendApproverLink
	{
	protected:
		bool try_approve(SpendRequest& req) override
		{
			if (req.amount_cents <= 10'000)
			{
				req.approved_by_role = "junior";
				return true;
			}
			return false;
		}
	};

	class LeadSpendApprover final : public SpendApproverLink
	{
	protected:
		bool try_approve(SpendRequest& req) override
		{
			if (req.amount_cents <= 100'000)
			{
				req.approved_by_role = "lead";
				return true;
			}
			return false;
		}
	};

	class DirectorSpendApprover final : public SpendApproverLink
	{
	protected:
		bool try_approve(SpendRequest& req) override
		{
			(void)req.amount_cents;
			req.approved_by_role = "director";
			return true;
		}
	};

	inline void link_spend_chain(JuniorSpendApprover& j, LeadSpendApprover& l, DirectorSpendApprover& d)
	{
		j.set_next(&l);
		l.set_next(&d);
	}

	// -----------------------------------------------------------------
	// Example 2 — Support routing by topic keyword (case-insensitive contains).
	// -----------------------------------------------------------------
	struct SupportTicket
	{
		std::string topic;
		std::string routed_queue;
	};

	static bool icontains(std::string_view hay, std::string_view needle)
	{
		std::string h(hay);
		std::string n(needle);
		std::transform(h.begin(), h.end(), h.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return h.find(n) != std::string::npos;
	}

	class SupportRouterLink
	{
	public:
		virtual ~SupportRouterLink() = default;

		void set_next(SupportRouterLink* successor) noexcept { next_ = successor; }

		void handle(SupportTicket& ticket)
		{
			if (try_route(ticket)) return;
			if (next_) next_->handle(ticket);
		}

	protected:
		virtual bool try_route(SupportTicket& ticket) = 0;

	private:
		SupportRouterLink* next_{};
	};

	class PasswordSupportDesk final : public SupportRouterLink
	{
	protected:
		bool try_route(SupportTicket& ticket) override
		{
			if (icontains(ticket.topic, "password"))
			{
				ticket.routed_queue = "identity_l1";
				return true;
			}
			return false;
		}
	};

	class BillingSupportDesk final : public SupportRouterLink
	{
	protected:
		bool try_route(SupportTicket& ticket) override
		{
			if (icontains(ticket.topic, "invoice") || icontains(ticket.topic, "bill"))
			{
				ticket.routed_queue = "billing_ops";
				return true;
			}
			return false;
		}
	};

	class DefaultEscalationDesk final : public SupportRouterLink
	{
	protected:
		bool try_route(SupportTicket& ticket) override
		{
			(void)ticket.topic;
			ticket.routed_queue = "general_triage";
			return true;
		}
	};

	inline void link_support_chain(PasswordSupportDesk& p, BillingSupportDesk& b, DefaultEscalationDesk& d)
	{
		p.set_next(&b);
		b.set_next(&d);
	}

	// -----------------------------------------------------------------
	// Example 3 — Audit decorator link: never "owns" the request, only records.
	// -----------------------------------------------------------------
	class AuditingSpendWrapper final : public SpendApproverLink
	{
	public:
		explicit AuditingSpendWrapper(SpendApproverLink& inner) : inner_(&inner) {}

		std::vector<std::string> audit;

	protected:
		bool try_approve(SpendRequest& req) override
		{
			audit.push_back("seen_amount=" + std::to_string(req.amount_cents));
			inner_->handle(req);
			return true;
		}

	private:
		SpendApproverLink* inner_{};
	};

	// -----------------------------------------------------------------
	// Example 4 — Linear chain as a table of predicates (functional style).
	// -----------------------------------------------------------------
	inline bool dispatch_with_first_match(std::string& value, const std::vector<std::function<bool(std::string&)>>& stages)
	{
		for (const auto& stage : stages)
			if (stage(value)) return true;
		return false;
	}

} // namespace usage_examples::patterns::behavioral::chain_of_responsibility_gof

namespace {

	using usage_examples::patterns::behavioral::chain_of_responsibility_gof::AuditingSpendWrapper;
	using usage_examples::patterns::behavioral::chain_of_responsibility_gof::BillingSupportDesk;
	using usage_examples::patterns::behavioral::chain_of_responsibility_gof::DefaultEscalationDesk;
	using usage_examples::patterns::behavioral::chain_of_responsibility_gof::DirectorSpendApprover;
	using usage_examples::patterns::behavioral::chain_of_responsibility_gof::dispatch_with_first_match;
	using usage_examples::patterns::behavioral::chain_of_responsibility_gof::JuniorSpendApprover;
	using usage_examples::patterns::behavioral::chain_of_responsibility_gof::LeadSpendApprover;
	using usage_examples::patterns::behavioral::chain_of_responsibility_gof::link_spend_chain;
	using usage_examples::patterns::behavioral::chain_of_responsibility_gof::link_support_chain;
	using usage_examples::patterns::behavioral::chain_of_responsibility_gof::PasswordSupportDesk;
	using usage_examples::patterns::behavioral::chain_of_responsibility_gof::SpendRequest;
	using usage_examples::patterns::behavioral::chain_of_responsibility_gof::SupportTicket;

	TEST(ChainOfResponsibilityUsageExamples, SpendRoutedToCorrectRole)
	{
		JuniorSpendApprover junior;
		LeadSpendApprover lead;
		DirectorSpendApprover director;
		link_spend_chain(junior, lead, director);

		SpendRequest small{ 2'500, {} };
		junior.handle(small);
		EXPECT_EQ(small.approved_by_role, "junior");

		SpendRequest mid{ 55'000, {} };
		junior.handle(mid);
		EXPECT_EQ(mid.approved_by_role, "lead");

		SpendRequest big{ 9'999'999, {} };
		junior.handle(big);
		EXPECT_EQ(big.approved_by_role, "director");
	}

	TEST(ChainOfResponsibilityUsageExamples, SupportTopicRoutesOrFallsThrough)
	{
		PasswordSupportDesk passwords;
		BillingSupportDesk billing;
		DefaultEscalationDesk fallback;
		link_support_chain(passwords, billing, fallback);

		SupportTicket t1{ "Forgot my PASSWORD reset", {} };
		passwords.handle(t1);
		EXPECT_EQ(t1.routed_queue, "identity_l1");

		SupportTicket t2{ "Invoice #12 looks wrong", {} };
		passwords.handle(t2);
		EXPECT_EQ(t2.routed_queue, "billing_ops");

		SupportTicket t3{ "Printer on fire (again)", {} };
		passwords.handle(t3);
		EXPECT_EQ(t3.routed_queue, "general_triage");
	}

	TEST(ChainOfResponsibilityUsageExamples, AuditWrapperRunsBeforeInnerChain)
	{
		JuniorSpendApprover junior;
		LeadSpendApprover lead;
		DirectorSpendApprover director;
		link_spend_chain(junior, lead, director);

		AuditingSpendWrapper audited(junior);
		SpendRequest req{ 500, {} };
		audited.handle(req);
		EXPECT_EQ(req.approved_by_role, "junior");
		ASSERT_EQ(audited.audit.size(), 1u);
		EXPECT_NE(audited.audit[0].find("500"), std::string::npos);
	}

	TEST(ChainOfResponsibilityUsageExamples, FunctionalDispatchTable)
	{
		std::string buffer = "raw";
		bool third_ran = false;
		const bool handled = dispatch_with_first_match(
			buffer,
			{
				[](std::string& s) {
					if (s == "alpha") { s = "A"; return true; }
					return false;
				},
				[](std::string& s) {
					if (s == "raw") { s = "normalized"; return true; }
					return false;
				},
				[&third_ran](std::string& s) {
					third_ran = true;
					(void)s;
					return true;
				},
			});
		EXPECT_TRUE(handled);
		EXPECT_EQ(buffer, "normalized");
		EXPECT_FALSE(third_ran);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add a **cyclic guard** (`thread_local` depth counter) if handlers might
 *    re-enter the same chain through callbacks.
 * 2. Build the chain from **config** (JSON list of handler ids) and a **factory**
 *    map instead of hard-coded `link_*` helpers.
 * 3. Model **async** CoR: each stage returns `std::future<bool>` and the client
 *    resumes when a stage claims the request or the list ends.
 * 4. Compare with **Boost.Beast / HTTP** filter stacks or **OpenTelemetry**
 *     processors — note where frameworks use exclusive vs all-handlers chains.
 */
