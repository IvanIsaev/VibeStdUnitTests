/*
 * =============================================================================
 * State (Gang of Four — Behavioral)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Allow an object to **alter its behavior** when its **internal state** changes.
 * The object will appear to **change its class** from the client’s perspective.
 *
 * You model each major state as a **separate type** (or a distinct branch in a
 * transition table). The **Context** delegates operations to the **current State**
 * object; **transitions** replace that pointer with the **next** state instance.
 *
 * Core pieces
 * -----------
 *   • **Context** — what clients hold (`TCPConnection`, `MediaPlayer`, `Order`).
 *     Exposes high-level operations (`open`, `play`, `submit`) and holds a
 *     `State*` / `unique_ptr<State>` / small handle to the active state.
 *
 *   • **State interface** — declares behavior hooks for each **event** the
 *     context can receive (`handle_event_a`, `play`, `coin_inserted`, …).
 *
 *   • **Concrete states** — implement those hooks; each method either **stays**
 *     in place, **transitions** the context to another state, or **delegates**
 *     part of the work back to the context (carefully, to avoid cycles).
 *
 *   • **Client / environment** — drives events (“user pressed Pause”, “ACK
 *     received”, “timer fired”). The client usually talks only to the context.
 *
 * Why not just a switch on an enum?
 * ---------------------------------
 * A giant `switch (phase)` works for **small** machines, but the State pattern
 * shines when:
 *
 *   • Each state has **non-trivial rules** and you want them **isolated** in
 *     their own type (easier to read and test than one mega-function).
 *
 *   • You add states often — **open/closed** modification per state beats
 *     editing a central switch that everyone merges on.
 *
 *   • You need **polymorphic** state-specific helpers (enter/exit hooks,
 *     optional substates, shared base for “connected” substates).
 *
 * Trade-offs
 * ----------
 *   • **More types** — many little classes vs one table; choose based on team
 *     taste and machine size.
 *
 *   • **Transition scattering** — logic lives in state methods; ensure **diagrams
 *     or generated tables** stay in sync with code reviews.
 *
 *   • **Lifetime** — states are often **flyweights** (static singletons) or
 *     **value objects** owned by a factory; avoid capturing **stale** context
 *     pointers in async callbacks.
 *
 * Related patterns
 * ----------------
 *   • **Strategy** — both compose behavior; **Strategy** is usually chosen by
 *     the **client** for **variation**, **State** transitions **internally**
 *     as the object’s phase changes.
 *
 *   • **Bridge** — separates abstraction from implementation; **State** focuses
 *     on **phase-dependent** behavior of one abstraction.
 *
 *   • **Memento** — save/restore context state across undo or snapshots;
 *     sometimes paired when leaving a state must roll back side effects.
 *
 * Implementation flavors
 * ----------------------
 *   • **Polymorphic state objects** (this file) — `Context::op()` forwards to
 *     `state_->op(*this)`; states call `ctx.transition_to(&Next::instance())`.
 *
 *   • **Table-driven** — `std::map<std::pair<StateId, EventId>, StateId>` or a
 *     sparse table; **actions** live in side functions. Great for generated
 *     specs; weaker compile-time exhaustiveness.
 *
 *   • **`std::variant` + `std::visit`** — closed set of states as alternatives;
 *     transitions return the **next** `variant` value (functional style).
 *
 *   • **Hierarchical states** — outer “Connected” state owns inner “Reading” /
 *     “Writing”; events bubble or delegate to the inner state first.
 *
 * Where it shows up
 * -----------------
 *   • **Networking stacks** — TCP LISTEN / ESTABLISHED / TIME_WAIT style FSMs.
 *   • **Media / games** — menus, AI behaviors, animation controllers.
 *   • **UI workflows** — wizards, checkout steps, disabled buttons per step.
 *   • **Devices** — power modes, ARM Cortex sleep levels, industrial controllers.
 *
 * Pitfalls
 * --------
 *   • **Illegal transitions** — define whether they are **ignored**, **assert**,
 *     or **logged**; document for API users.
 *
 *   • **Re-entrancy** — an event handler triggers another event on the same
 *     context; guard with **queues** or **“transition pending”** flags.
 *
 *   • **Enter/exit side effects** — starting a timer when *entering* “Playing”
 *     belongs in a dedicated `on_enter` if you split lifecycle hooks.
 *
 *   • **One-definition rule** — this repo links many demos into
 *     `usage_examples`; types live in `state_gof` so names like `Playing` or
 *     `Stopped` do not collide with other translation units.
 *
 * Testing
 * -------
 *   • **Walk scenarios** — drive a sequence of events; assert **state id** and
 *     any **context fields** (counters, buffers).
 *
 *   • **Coverage matrix** — for small machines, enumerate **(from, event) → to**.
 *
 *   • **Illegal input** — ensure **no crash** and stable state when events are
 *     ignored in a terminal state.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <type_traits>
#include <variant>

namespace usage_examples::patterns::behavioral::state_gof {

	// -----------------------------------------------------------------
	// Example 1 — Media playback FSM: Stopped ↔ Playing ↔ Paused
	// -----------------------------------------------------------------
	class MediaPlayer;

	struct IPlaybackState
	{
		virtual ~IPlaybackState() = default;
		virtual void play(MediaPlayer& ctx) = 0;
		virtual void pause(MediaPlayer& ctx) = 0;
		virtual void stop(MediaPlayer& ctx) = 0;
		[[nodiscard]] virtual const char* id() const noexcept = 0;
	};

	class MediaPlayer
	{
	public:
		MediaPlayer();

		void set_state(IPlaybackState* next) noexcept { state_ = next; }

		[[nodiscard]] IPlaybackState* current_state() const noexcept { return state_; }

		void play() { state_->play(*this); }
		void pause() { state_->pause(*this); }
		void stop() { state_->stop(*this); }

	private:
		IPlaybackState* state_{};
	};

	struct StoppedState final : IPlaybackState
	{
		void play(MediaPlayer& ctx) override;
		void pause(MediaPlayer& ctx) override {}
		void stop(MediaPlayer& ctx) override {}
		[[nodiscard]] const char* id() const noexcept override { return "stopped"; }
		static StoppedState& instance() noexcept;
	};

	struct PlayingState final : IPlaybackState
	{
		void play(MediaPlayer& ctx) override;
		void pause(MediaPlayer& ctx) override;
		void stop(MediaPlayer& ctx) override;
		[[nodiscard]] const char* id() const noexcept override { return "playing"; }
		static PlayingState& instance() noexcept;
	};

	struct PausedState final : IPlaybackState
	{
		void play(MediaPlayer& ctx) override;
		void pause(MediaPlayer& ctx) override {}
		void stop(MediaPlayer& ctx) override;
		[[nodiscard]] const char* id() const noexcept override { return "paused"; }
		static PausedState& instance() noexcept;
	};

	inline StoppedState& StoppedState::instance() noexcept
	{
		static StoppedState s;
		return s;
	}

	inline PlayingState& PlayingState::instance() noexcept
	{
		static PlayingState s;
		return s;
	}

	inline PausedState& PausedState::instance() noexcept
	{
		static PausedState s;
		return s;
	}

	inline void StoppedState::play(MediaPlayer& ctx) { ctx.set_state(&PlayingState::instance()); }

	inline void PlayingState::play(MediaPlayer& ctx) { (void)ctx; }

	inline void PlayingState::pause(MediaPlayer& ctx) { ctx.set_state(&PausedState::instance()); }

	inline void PlayingState::stop(MediaPlayer& ctx) { ctx.set_state(&StoppedState::instance()); }

	inline void PausedState::play(MediaPlayer& ctx) { ctx.set_state(&PlayingState::instance()); }

	inline void PausedState::stop(MediaPlayer& ctx) { ctx.set_state(&StoppedState::instance()); }

	inline MediaPlayer::MediaPlayer() : state_(&StoppedState::instance()) {}

	// -----------------------------------------------------------------
	// Example 2 — Lamp toggle: two-state machine (on ↔ off)
	// -----------------------------------------------------------------
	class Lamp;

	struct ILampState
	{
		virtual ~ILampState() = default;
		virtual void toggle(Lamp& lamp) = 0;
		[[nodiscard]] virtual const char* id() const noexcept = 0;
	};

	class Lamp
	{
	public:
		Lamp();

		void set_state(ILampState* next) noexcept { state_ = next; }

		[[nodiscard]] ILampState* current_state() const noexcept { return state_; }

		void toggle() { state_->toggle(*this); }

	private:
		ILampState* state_{};
	};

	struct LampOffState final : ILampState
	{
		void toggle(Lamp& lamp) override;
		[[nodiscard]] const char* id() const noexcept override { return "lamp_off"; }
		static LampOffState& instance() noexcept;
	};

	struct LampOnState final : ILampState
	{
		void toggle(Lamp& lamp) override;
		[[nodiscard]] const char* id() const noexcept override { return "lamp_on"; }
		static LampOnState& instance() noexcept;
	};

	inline LampOffState& LampOffState::instance() noexcept
	{
		static LampOffState s;
		return s;
	}

	inline LampOnState& LampOnState::instance() noexcept
	{
		static LampOnState s;
		return s;
	}

	inline void LampOffState::toggle(Lamp& lamp) { lamp.set_state(&LampOnState::instance()); }

	inline void LampOnState::toggle(Lamp& lamp) { lamp.set_state(&LampOffState::instance()); }

	inline Lamp::Lamp() : state_(&LampOffState::instance()) {}

	// -----------------------------------------------------------------
	// Example 3 — Support ticket: Open → InProgress → Closed (terminal)
	// -----------------------------------------------------------------
	class HelpTicket;

	struct ITicketState
	{
		virtual ~ITicketState() = default;
		virtual void start(HelpTicket& ticket) {}
		virtual void resolve(HelpTicket& ticket) {}
		[[nodiscard]] virtual const char* id() const noexcept = 0;
	};

	class HelpTicket
	{
	public:
		HelpTicket();

		void set_state(ITicketState* next) noexcept { state_ = next; }

		[[nodiscard]] ITicketState* current_state() const noexcept { return state_; }

		void start() { state_->start(*this); }
		void resolve() { state_->resolve(*this); }

	private:
		ITicketState* state_{};
	};

	struct TicketOpenState final : ITicketState
	{
		void start(HelpTicket& ticket) override;
		[[nodiscard]] const char* id() const noexcept override { return "ticket_open"; }
		static TicketOpenState& instance() noexcept;
	};

	struct TicketInProgressState final : ITicketState
	{
		void resolve(HelpTicket& ticket) override;
		[[nodiscard]] const char* id() const noexcept override { return "ticket_progress"; }
		static TicketInProgressState& instance() noexcept;
	};

	struct TicketClosedState final : ITicketState
	{
		[[nodiscard]] const char* id() const noexcept override { return "ticket_closed"; }
		static TicketClosedState& instance() noexcept;
	};

	inline TicketOpenState& TicketOpenState::instance() noexcept
	{
		static TicketOpenState s;
		return s;
	}

	inline TicketInProgressState& TicketInProgressState::instance() noexcept
	{
		static TicketInProgressState s;
		return s;
	}

	inline TicketClosedState& TicketClosedState::instance() noexcept
	{
		static TicketClosedState s;
		return s;
	}

	inline void TicketOpenState::start(HelpTicket& ticket)
	{
		ticket.set_state(&TicketInProgressState::instance());
	}

	inline void TicketInProgressState::resolve(HelpTicket& ticket)
	{
		ticket.set_state(&TicketClosedState::instance());
	}

	inline HelpTicket::HelpTicket() : state_(&TicketOpenState::instance()) {}

	// -----------------------------------------------------------------
	// Example 4 — std::variant state (closed set, no virtuals)
	// -----------------------------------------------------------------
	enum class TurnstileEvent { coin, push };

	struct TurnstileLocked
	{
	};
	struct TurnstileUnlocked
	{
	};

	using TurnstileState = std::variant<TurnstileLocked, TurnstileUnlocked>;

	inline TurnstileState dispatch_turnstile(TurnstileState current, TurnstileEvent ev)
	{
		return std::visit(
			[ev](auto&& s) -> TurnstileState {
				using T = std::decay_t<decltype(s)>;
				if constexpr (std::is_same_v<T, TurnstileLocked>)
				{
					if (ev == TurnstileEvent::coin) return TurnstileUnlocked{};
					return TurnstileLocked{};
				}
				else
				{
					if (ev == TurnstileEvent::push) return TurnstileLocked{};
					return TurnstileUnlocked{};
				}
			},
			current);
	}

} // namespace usage_examples::patterns::behavioral::state_gof

namespace {

	using usage_examples::patterns::behavioral::state_gof::dispatch_turnstile;
	using usage_examples::patterns::behavioral::state_gof::HelpTicket;
	using usage_examples::patterns::behavioral::state_gof::Lamp;
	using usage_examples::patterns::behavioral::state_gof::MediaPlayer;
	using usage_examples::patterns::behavioral::state_gof::PausedState;
	using usage_examples::patterns::behavioral::state_gof::PlayingState;
	using usage_examples::patterns::behavioral::state_gof::StoppedState;
	using usage_examples::patterns::behavioral::state_gof::TurnstileEvent;
	using usage_examples::patterns::behavioral::state_gof::TurnstileLocked;
	using usage_examples::patterns::behavioral::state_gof::TurnstileState;
	using usage_examples::patterns::behavioral::state_gof::TurnstileUnlocked;

	TEST(StateUsageExamples, MediaPlayerStoppedToPlayingToPaused)
	{
		MediaPlayer p;
		EXPECT_STREQ(p.current_state()->id(), StoppedState::instance().id());
		p.play();
		EXPECT_STREQ(p.current_state()->id(), PlayingState::instance().id());
		p.pause();
		EXPECT_STREQ(p.current_state()->id(), PausedState::instance().id());
		p.play();
		EXPECT_STREQ(p.current_state()->id(), PlayingState::instance().id());
	}

	TEST(StateUsageExamples, MediaPlayerStopFromPlayingReturnsToStopped)
	{
		MediaPlayer p;
		p.play();
		p.stop();
		EXPECT_STREQ(p.current_state()->id(), StoppedState::instance().id());
	}

	TEST(StateUsageExamples, MediaPlayerStopWhenStoppedIsIgnored)
	{
		MediaPlayer p;
		p.stop();
		EXPECT_STREQ(p.current_state()->id(), StoppedState::instance().id());
	}

	TEST(StateUsageExamples, LampTogglesOnAndOff)
	{
		Lamp lamp;
		EXPECT_STREQ(lamp.current_state()->id(), "lamp_off");
		lamp.toggle();
		EXPECT_STREQ(lamp.current_state()->id(), "lamp_on");
		lamp.toggle();
		EXPECT_STREQ(lamp.current_state()->id(), "lamp_off");
	}

	TEST(StateUsageExamples, HelpTicketLifecycleToClosed)
	{
		HelpTicket t;
		EXPECT_STREQ(t.current_state()->id(), "ticket_open");
		t.resolve();
		EXPECT_STREQ(t.current_state()->id(), "ticket_open");
		t.start();
		EXPECT_STREQ(t.current_state()->id(), "ticket_progress");
		t.start();
		EXPECT_STREQ(t.current_state()->id(), "ticket_progress");
		t.resolve();
		EXPECT_STREQ(t.current_state()->id(), "ticket_closed");
		t.resolve();
		EXPECT_STREQ(t.current_state()->id(), "ticket_closed");
	}

	TEST(StateUsageExamples, TurnstileVariantStyleDispatch)
	{
		TurnstileState s{ TurnstileLocked{} };
		s = dispatch_turnstile(s, TurnstileEvent::push);
		EXPECT_TRUE(std::holds_alternative<TurnstileLocked>(s));
		s = dispatch_turnstile(s, TurnstileEvent::coin);
		EXPECT_TRUE(std::holds_alternative<TurnstileUnlocked>(s));
		s = dispatch_turnstile(s, TurnstileEvent::coin);
		EXPECT_TRUE(std::holds_alternative<TurnstileUnlocked>(s));
		s = dispatch_turnstile(s, TurnstileEvent::push);
		EXPECT_TRUE(std::holds_alternative<TurnstileLocked>(s));
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add **on_enter / on_exit** hooks on `IPlaybackState` and log transitions
 *    for diagnostics (match sequence diagrams).
 * 2. Model a **hierarchical** player: outer `PoweredOn` vs `PoweredOff`, inner
 *    `Playing`/`Paused` only when powered on.
 * 3. Serialize **current state id** to JSON and rebuild the same machine after
 *    restart (migration when enum values change).
 * 4. Replace singleton states with a **factory** that shares flyweights but
 *    allows per-state configuration (timeouts, labels).
 */
