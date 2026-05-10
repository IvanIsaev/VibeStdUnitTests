/*
 * =============================================================================
 * Observer (Gang of Four — Behavioral)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Define a **one-to-many dependency** between objects so that when one object
 * changes state, **all dependents are notified and updated** automatically.
 *
 * The **Subject** (or “observable”) knows its **Observers** and broadcasts
 * events. Observers **subscribe** once and react whenever the subject publishes,
 * without the subject enumerating concrete subscriber types.
 *
 * Typical structure
 * -----------------
 *   • **Subject** — `attach` / `detach` / `notify`; often holds observer handles.
 *   • **Observer** — interface with `update(...)` or domain-specific callbacks.
 *   • **ConcreteSubject** — stores real state; calls `notify` after mutations.
 *   • **ConcreteObserver** — refreshes UI, cache, logs, or downstream services.
 *
 * Push vs pull
 * ------------
 *   • **Push** — subject passes **arguments** in `notify` (`on_price_changed(99)`).
 *     Simple; avoid huge parameter lists (use a small event struct).
 *
 *   • **Pull** — subject only signals “I changed”; observers **query** the
 *     subject for details. Looser coupling but can race if state changes twice
 *     between pull and use (document threading).
 *
 * Why use it
 * ----------
 *   • **Decouple** producers from consumers — add analytics or a second UI without
 *     editing the core model for each new listener.
 *
 *   • **Event-driven** workflows — domain events fan out to many handlers.
 *
 *   • **GUI and reactive** patterns — MVC presenters, data binding, reactive
 *     streams (Rx) generalize the same idea.
 *
 * C++ implementation options
 * ----------------------------
 *   • **Virtual `IObserver`** + subject stores `vector<Observer*>` — classic;
 *     document **lifetime** (who owns observers, dangling pointers).
 *
 *   • **`std::function` / lambdas** — lightweight; easy for tests; harder to
 *     `detach` a specific lambda unless you store tokens or `connection` handles.
 *
 *   • **`std::weak_ptr`** observers + `shared_ptr` subject — mitigates cycles;
 *     notify must lock each weak_ptr and drop expired entries.
 *
 *   • **Libraries** — Qt signals/slots, `boost::signals2`, home-grown event buses.
 *
 * Observer vs related ideas
 * -------------------------
 *   • **Pub/Sub** (message broker) often adds **async** delivery and **topic**
 *     names; Observer is usually **in-process** and synchronous unless queued.
 *
 *   • **Chain of Responsibility** passes a request along a **chain** until handled;
 *     Observer **broadcasts** to **all** subscribers.
 *
 *   • **Mediator** routes communication **through** a hub; Observer is direct
 *     subject-to-many pattern (a mediator can own subjects/observers in larger
 *     systems).
 *
 * Pitfalls
 * --------
 *   • **Re-entrancy** — `notify` may call back into the subject; avoid modifying
 *     the observer list during iteration (copy listeners first, or defer adds).
 *
 *   • **Order dependence** — if handlers rely on notification order, document
 *     it or sort by explicit priority.
 *
 *   • **Leaks and dangling** — raw observer pointers require clear ownership;
 *     destroyed observers **must** unsubscribe or use weak handles.
 *
 *   • **Thread safety** — protect the subscriber list and subject state with
 *     mutexes or post events to a single-threaded queue.
 *
 * Testing
 * -------
 *   • Use **spy** observers that record payloads.
 *   • Verify **detach** stops delivery without affecting other subscribers.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace usage_examples::patterns::behavioral {

	// -----------------------------------------------------------------
	// Classic interface-based Observer.
	// -----------------------------------------------------------------
	struct INewsObserver
	{
		virtual ~INewsObserver() = default;
		virtual void on_headline(std::string_view headline) = 0;
	};

	class NewsPublisher
	{
	public:
		void subscribe(INewsObserver* observer)
		{
			if (!observer) return;
			if (std::find(observers_.begin(), observers_.end(), observer) != observers_.end()) return;
			observers_.push_back(observer);
		}

		void unsubscribe(INewsObserver* observer)
		{
			observers_.erase(std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
		}

		void publish(std::string_view headline)
		{
			// Snapshot so re-entrant subscribe or unsubscribe during notify is safer.
			const auto snapshot = observers_;
			for (INewsObserver* o : snapshot)
				o->on_headline(headline);
		}

	private:
		std::vector<INewsObserver*> observers_;
	};

	struct HeadlineSpy final : INewsObserver
	{
		std::string last_headline;
		int call_count = 0;

		void on_headline(std::string_view headline) override
		{
			last_headline = std::string(headline);
			++call_count;
		}
	};

	// -----------------------------------------------------------------
	// Functional observers — good for small callbacks; detach is by token or
	// by clearing (here: replace vector in tests only).
	// -----------------------------------------------------------------
	class IntegerBroadcaster
	{
	public:
		using Listener = std::function<void(int)>;

		void add_listener(Listener fn) { listeners_.push_back(std::move(fn)); }

		void clear_listeners() { listeners_.clear(); }

		void broadcast(int value) const
		{
			for (const Listener& fn : listeners_)
				fn(value);
		}

		[[nodiscard]] std::size_t listener_count() const { return listeners_.size(); }

	private:
		std::vector<Listener> listeners_;
	};

} // namespace usage_examples::patterns::behavioral

namespace {

	using usage_examples::patterns::behavioral::HeadlineSpy;
	using usage_examples::patterns::behavioral::IntegerBroadcaster;
	using usage_examples::patterns::behavioral::NewsPublisher;

	TEST(ObserverUsageExamples, AllSubscribersReceiveHeadlines)
	{
		NewsPublisher desk;
		HeadlineSpy a;
		HeadlineSpy b;
		desk.subscribe(&a);
		desk.subscribe(&b);

		desk.publish("Storm warning");

		EXPECT_EQ(a.last_headline, "Storm warning");
		EXPECT_EQ(b.last_headline, "Storm warning");
		EXPECT_EQ(a.call_count, 1);
		EXPECT_EQ(b.call_count, 1);
	}

	TEST(ObserverUsageExamples, UnsubscribedObserverStopsReceiving)
	{
		NewsPublisher desk;
		HeadlineSpy a;
		HeadlineSpy b;
		desk.subscribe(&a);
		desk.subscribe(&b);
		desk.unsubscribe(&a);

		desk.publish("Quiet day");

		EXPECT_EQ(a.last_headline, "");
		EXPECT_EQ(a.call_count, 0);
		EXPECT_EQ(b.last_headline, "Quiet day");
	}

	TEST(ObserverUsageExamples, FunctionalListenersFanOut)
	{
		IntegerBroadcaster hub;
		int sum = 0;
		int product = 1;
		hub.add_listener([&sum](int v) { sum += v; });
		hub.add_listener([&product](int v) { product *= v; });

		hub.broadcast(3);
		hub.broadcast(4);

		EXPECT_EQ(sum, 7);
		EXPECT_EQ(product, 12);
		EXPECT_EQ(hub.listener_count(), 2u);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Store `std::weak_ptr<INewsObserver>` in `NewsPublisher` and prune expired
 *    locks inside `publish`.
 * 2. Replace string headlines with a small `struct NewsEvent { std::string title;
 *    std::chrono::system_clock::time_point when; }` (push model).
 * 3. Queue notifications on a worker thread with a lock-free MPMC queue and compare
 *    latency with synchronous Observer calls under load.
 */
