/*
 * =============================================================================
 * Mediator (Gang of Four — Behavioral)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Define an object that **encapsulates how a set of objects interact**. Mediator
 * promotes **loose coupling** by keeping objects from referring to each other
 * explicitly, and lets you vary their interaction **independently**.
 *
 * Instead of an **N×N** mesh of peer references (“every dialog widget knows
 * every other widget”), collaborators (**Colleagues**) talk only to a **Mediator**
 * hub that routes, filters, or sequences messages.
 *
 * Participants
 * ------------
 *   • **Mediator** — defines the interface for communicating with colleague
 *     objects (`notify`, `send`, `field_changed`, …).
 *
 *   • **ConcreteMediator** — coordinates colleagues; often keeps **central
 *     state** (who is selected, which runway is busy, validation snapshot).
 *
 *   • **Colleague classes** — each knows its mediator (usually one reference);
 *     never calls other colleagues **directly** for coordinated workflows.
 *
 *   • **Client** — creates colleagues + wires them to the mediator; may trigger
 *     first events.
 *
 * What problems it solves
 * -----------------------
 *   • **Exploding coupling** — GUI forms, multiplayer lobbies, plugin hosts, and
 *     micro-frontends all tend toward “everyone imports everyone.” A hub **cuts
 *     edges** from O(N²) toward O(N).
 *
 *   • **Centralized policy** — validation rules, feature flags, or arbitration
 *     live in **one** place instead of duplicated across peers.
 *
 *   • **Easier testing** — replace the mediator with a **test double** that
 *     records calls or drives scripted responses.
 *
 * Costs and pitfalls
 * ------------------
 *   • **God object risk** — if *all* domain logic migrates into the mediator,
 *     it becomes a **monolith**. Split by **sub-mediators** or **domains**
 *     (chat vs roster vs moderation).
 *
 *   • **Indirection** — harder to trace control flow than direct calls; lean on
 *     **logging**, **sequence diagrams**, and **narrow mediator interfaces**.
 *
 *   • **Performance** — a hot path that bounced through a mediator per pixel can
 *     hurt; profile and **batch** notifications (coalesce text edits).
 *
 * Related patterns
 * ----------------
 *   • **Observer** — many subjects notify many observers **without** a central
 *     router; Mediator **orchestrates** and may **suppress**, **reorder**, or
 *     **transform** events.
 *
 *   • **Facade** — simplifies a **subsystem** API for outsiders; Mediator focuses
 *     on **peer-to-peer coordination** inside a cluster of objects.
 *
 *   • **Command** — colleagues emit commands; mediator enqueues / undoes them.
 *
 *   • **State** — mediator may own or consult a **finite-state machine** for
 *     dialog modes (read-only vs editing).
 *
 * Implementation flavors
 * ----------------------
 *   • **OO hub** — virtual `IMediator` + colleague base (this file’s style).
 *
 *   • **Message bus** — topic strings, async queues, middleware (Kafka, in-proc
 *     `ConcurrentQueue`), idempotent handlers.
 *
 *   • **Reactive stores** — Redux-style single store + reducers are a **global
 *     mediator** for UI state (with strong conventions to avoid the god-object
 *     smell).
 *
 * Testing
 * -------
 *   • **Spy mediator** — record `notify` arguments; assert ordering and counts.
 *
 *   • **Scenario tests** — “user A whispers B” / “runway busy” end-to-end via
 *     public colleague APIs only.
 *
 *   • **One-definition rule** — types live in `mediator_gof` so this demo does
 *     not collide with other `usage_examples` sources.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace usage_examples::patterns::behavioral::mediator_gof {

	// -----------------------------------------------------------------
	// Example 1 — Chat room: colleagues only send to the room mediator
	// -----------------------------------------------------------------
	class ChatRoom;

	class ChatMember
	{
	public:
		virtual ~ChatMember() = default;

		ChatMember(std::string display_name, ChatRoom* room) : name_(std::move(display_name)), room_(room) {}

		[[nodiscard]] const std::string& name() const { return name_; }

		void send_broadcast(std::string message);

		virtual void receive(std::string_view from_display_name, std::string_view text) = 0;

	protected:
		ChatRoom* room() const noexcept { return room_; }

	private:
		std::string name_;
		ChatRoom* room_{};
	};

	class ChatRoom
	{
	public:
		void join(ChatMember& member) { members_.push_back(&member); }

		void relay_broadcast(const ChatMember& sender, std::string_view text)
		{
			for (ChatMember* peer : members_)
			{
				if (peer == &sender) continue;
				peer->receive(sender.name(), text);
			}
		}

	private:
		std::vector<ChatMember*> members_;
	};

	inline void ChatMember::send_broadcast(std::string message)
	{
		if (!room_) return;
		room_->relay_broadcast(*this, message);
	}

	class InboxChatMember final : public ChatMember
	{
	public:
		InboxChatMember(std::string display_name, ChatRoom* room) : ChatMember(std::move(display_name), room) {}

		void receive(std::string_view from_display_name, std::string_view text) override
		{
			log_.push_back(std::string(from_display_name) + " says: " + std::string(text));
		}

		[[nodiscard]] const std::vector<std::string>& inbox() const { return log_; }

	private:
		std::vector<std::string> log_;
	};

	// -----------------------------------------------------------------
	// Example 2 — Single-runway tower: serializes takeoff clearance
	// -----------------------------------------------------------------
	class RunwayTower;

	class Aircraft
	{
	public:
		Aircraft(std::string callsign, RunwayTower* tower) : callsign_(std::move(callsign)), tower_(tower) {}

		[[nodiscard]] const std::string& callsign() const { return callsign_; }

		[[nodiscard]] bool has_clearance() const noexcept { return cleared_; }

		void request_takeoff();

		void release_after_departure();

	private:
		friend class RunwayTower;
		void grant_clearance() { cleared_ = true; }
		void revoke_clearance() { cleared_ = false; }

		std::string callsign_;
		RunwayTower* tower_{};
		bool cleared_{false};
	};

	class RunwayTower
	{
	public:
		void handle_takeoff_request(Aircraft& plane);

		void notify_departed(Aircraft& plane);

	private:
		std::vector<Aircraft*> waiting_;
		Aircraft* active_{};
	};

	inline void Aircraft::request_takeoff()
	{
		if (tower_) tower_->handle_takeoff_request(*this);
	}

	inline void Aircraft::release_after_departure()
	{
		if (tower_) tower_->notify_departed(*this);
	}

	inline void RunwayTower::handle_takeoff_request(Aircraft& plane)
	{
		if (!active_)
		{
			active_ = &plane;
			plane.grant_clearance();
			return;
		}
		waiting_.push_back(&plane);
	}

	inline void RunwayTower::notify_departed(Aircraft& plane)
	{
		if (active_ != &plane) return;
		plane.revoke_clearance();
		active_ = nullptr;
		if (waiting_.empty()) return;
		Aircraft* next = waiting_.front();
		waiting_.erase(waiting_.begin());
		active_ = next;
		next->grant_clearance();
	}

	// -----------------------------------------------------------------
	// Example 3 — Dialog mediator: enable Sign-in when both fields non-empty
	// -----------------------------------------------------------------
	enum class LoginFieldId { username, password };

	class SignInButtonColleague;

	class LoginDialogMediator
	{
	public:
		void bind_button(SignInButtonColleague* button) noexcept { button_ = button; }

		void notify_text_changed(LoginFieldId which, std::string value);

	private:
		void refresh_button_state();

		SignInButtonColleague* button_{};
		std::string username_;
		std::string password_;
	};

	class SignInButtonColleague
	{
	public:
		explicit SignInButtonColleague(LoginDialogMediator* mediator) : mediator_(mediator)
		{
			if (mediator_) mediator_->bind_button(this);
		}

		void set_enabled(bool enabled) noexcept { enabled_ = enabled; }

		[[nodiscard]] bool enabled() const noexcept { return enabled_; }

	private:
		LoginDialogMediator* mediator_{};
		bool enabled_{false};
	};

	class TextFieldColleague
	{
	public:
		TextFieldColleague(LoginDialogMediator* mediator, LoginFieldId which) : mediator_(mediator), which_(which) {}

		void set_text(std::string value)
		{
			if (mediator_) mediator_->notify_text_changed(which_, std::move(value));
		}

	private:
		LoginDialogMediator* mediator_{};
		LoginFieldId which_{};
	};

	inline void LoginDialogMediator::notify_text_changed(LoginFieldId which, std::string value)
	{
		if (which == LoginFieldId::username) username_ = std::move(value);
		else password_ = std::move(value);
		refresh_button_state();
	}

	inline void LoginDialogMediator::refresh_button_state()
	{
		if (!button_) return;
		button_->set_enabled(!username_.empty() && !password_.empty());
	}

} // namespace usage_examples::patterns::behavioral::mediator_gof

namespace {

	using usage_examples::patterns::behavioral::mediator_gof::Aircraft;
	using usage_examples::patterns::behavioral::mediator_gof::ChatRoom;
	using usage_examples::patterns::behavioral::mediator_gof::InboxChatMember;
	using usage_examples::patterns::behavioral::mediator_gof::LoginDialogMediator;
	using usage_examples::patterns::behavioral::mediator_gof::LoginFieldId;
	using usage_examples::patterns::behavioral::mediator_gof::RunwayTower;
	using usage_examples::patterns::behavioral::mediator_gof::SignInButtonColleague;
	using usage_examples::patterns::behavioral::mediator_gof::TextFieldColleague;

	TEST(MediatorUsageExamples, ChatRoomRelaysWithoutPeerPointers)
	{
		ChatRoom room;
		InboxChatMember alice("Alice", &room);
		InboxChatMember bob("Bob", &room);
		InboxChatMember carol("Carol", &room);
		room.join(alice);
		room.join(bob);
		room.join(carol);

		alice.send_broadcast("hello-all");

		ASSERT_EQ(bob.inbox().size(), 1u);
		EXPECT_EQ(bob.inbox().front(), "Alice says: hello-all");
		ASSERT_EQ(carol.inbox().size(), 1u);
		EXPECT_TRUE(alice.inbox().empty());
	}

	TEST(MediatorUsageExamples, RunwayTowerSerializesClearance)
	{
		RunwayTower tower;
		Aircraft delta("DAL-102", &tower);
		Aircraft united("UAL-441", &tower);

		delta.request_takeoff();
		united.request_takeoff();
		EXPECT_TRUE(delta.has_clearance());
		EXPECT_FALSE(united.has_clearance());

		delta.release_after_departure();
		EXPECT_FALSE(delta.has_clearance());
		EXPECT_TRUE(united.has_clearance());
	}

	TEST(MediatorUsageExamples, LoginMediatorEnablesButtonWhenFormComplete)
	{
		LoginDialogMediator dialog;
		SignInButtonColleague sign_in(&dialog);
		TextFieldColleague user(&dialog, LoginFieldId::username);
		TextFieldColleague pass(&dialog, LoginFieldId::password);

		EXPECT_FALSE(sign_in.enabled());
		user.set_text("ada");
		EXPECT_FALSE(sign_in.enabled());
		pass.set_text("secret");
		EXPECT_TRUE(sign_in.enabled());
		pass.set_text("");
		EXPECT_FALSE(sign_in.enabled());
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add **private messaging** to `ChatRoom` (`relay_direct(sender, target, msg)`)
 *    without letting `ChatMember` store `ChatMember*` peers.
 * 2. Split `LoginDialogMediator` into **validation strategies** (email regex,
 *    password strength) injected behind one `refresh_button_state()`.
 * 3. Publish domain events onto an **async bus**; make the mediator a thin
 *    adapter that subscribes and translates to colleague calls.
 * 4. Draw the **before/after** collaboration graph (N² edges vs star) for a
 *    6-widget dialog and discuss maintainability.
 */
