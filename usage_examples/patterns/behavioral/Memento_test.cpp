/*
 * =============================================================================
 * Memento (Gang of Four — Behavioral)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Without violating encapsulation, **capture and externalize** an object’s
 * **internal state** so the object can be **restored** to that state later.
 *
 * Think **undo/redo**, **save games**, **database savepoints**, or **draft**
 * snapshots: the object that owns the tricky invariants (**Originator**) knows
 * how to copy itself into a **Memento** token; something else (**Caretaker**)
 * stores tokens **without knowing their internals**.
 *
 * Participants
 * ------------
 *   • **Memento** — stores Originator state; **opaque** to everyone except the
 *     Originator (often `friend` or package-private). May expose **metadata** only
 *     (label, timestamp) to the Caretaker.
 *
 *   • **Originator** — creates a Memento from its fields; **restores** itself
 *     from a Memento; continues to own the *live* domain object.
 *
 *   • **Caretaker** — guards the Memento (stack, list, file, cloud slot) but
 *     must **not** mutate or interpret private payload; prevents accidental
 *     coupling.
 *
 * Why not just “struct Snapshot { public fields }”?
 * -------------------------------------------------
 * A wide-open snapshot **breaks encapsulation**: every caller can invent invalid
 * combinations (“cursor past end”, “negative balance + closed account”).
 * A narrow Memento **keeps invariants inside** the Originator while still
 * allowing **external storage** of history.
 *
 * Narrow vs wide interface
 * ------------------------
 *   • **Narrow (ideal)** — only Originator can read/write the memento’s payload;
 *     Caretaker stores `unique_ptr<Memento>` or opaque handles.
 *
 *   • **Wide (pragmatic)** — memento exposes read-only **introspection** for UI
 *     (“Save #3 — 12:04”) or **serialization** (`to_json`) when you accept weaker
 *     encapsulation for tooling or backwards compatibility.
 *
 * Relationship to Command
 * -----------------------
 * **Command** often **creates** mementos *before* mutating (`execute` pushes
 * pre-state), and **restores** them on `undo`. Memento answers *what to store*;
 * Command answers *when and how to replay*.
 *
 * Implementation strategies
 * -------------------------
 *   • **Full snapshot** — copy every field (simple; can be heavy for big graphs).
 *
 *   • **Incremental / delta** — store edits since last checkpoint (compact;
 *     harder to get right when operations don’t commute).
 *
 *   • **Serialization** — memento is a byte blob or JSON; version with a
 *     **schema id** for migrations.
 *
 *   • **Structural sharing** — persistent data structures or copy-on-write
 *     buffers so snapshots are cheap (Clojure-style vectors, rope trees).
 *
 * Pitfalls
 * --------
 *   • **Stale references** — restoring object graphs may **dangle** if other
 *     code holds raw pointers into freed subgraphs; pair with handles or
 *     explicit **invalidation** events.
 *
 *   • **Memory growth** — unbounded undo stacks; cap depth or **coalesce**
 *     micro-mementos (typing bursts).
 *
 *   • **Threading** — capture state under a **lock**; Caretaker storage may need
 *     its own synchronization.
 *
 *   • **One-definition rule** — demo types live in `memento_gof` alongside
 *     other `usage_examples` translation units.
 *
 * Testing
 * -------
 *   • **Round-trip** — `auto m = origin.save(); origin.mutate(); origin.load(m);`
 *     expect original observable state.
 *
 *   • **Caretaker isolation** — tests should not depend on **peeking** inside
 *     the memento unless you deliberately model a wide interface.
 *
 *   • **Versioning** — if serializing, add fixtures for **old** file formats.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace usage_examples::patterns::behavioral::memento_gof {

	// -----------------------------------------------------------------
	// Example 1 — Text + caret: memento is opaque; only editor reads it
	// -----------------------------------------------------------------
	class TextDocumentMemento
	{
		friend class TextDocument;

	public:
		[[nodiscard]] std::string_view label() const noexcept { return label_; }

		void set_label(std::string human_readable) { label_ = std::move(human_readable); }

	private:
		explicit TextDocumentMemento(std::string buffer, std::size_t caret, std::string label = {})
			: buffer_(std::move(buffer)), caret_(caret), label_(std::move(label))
		{
		}

		std::string buffer_{};
		std::size_t caret_{};
		std::string label_{};
	};

	class TextDocument
	{
	public:
		[[nodiscard]] const std::string& text() const noexcept { return buffer_; }

		[[nodiscard]] std::size_t caret() const noexcept { return caret_; }

		void set_caret(std::size_t position)
		{
			if (position > buffer_.size()) throw std::out_of_range("TextDocument::set_caret");
			caret_ = position;
		}

		void insert(std::string_view chunk)
		{
			buffer_.insert(caret_, chunk);
			caret_ += chunk.size();
		}

		void backspace()
		{
			if (caret_ == 0) return;
			buffer_.erase(caret_ - 1, 1);
			--caret_;
		}

		[[nodiscard]] std::unique_ptr<TextDocumentMemento> create_memento(std::string_view snapshot_label = {}) const
		{
			auto m = std::unique_ptr<TextDocumentMemento>(new TextDocumentMemento(buffer_, caret_, std::string(snapshot_label)));
			return m;
		}

		void restore(const TextDocumentMemento& snapshot)
		{
			buffer_ = snapshot.buffer_;
			caret_ = snapshot.caret_;
		}

	private:
		std::string buffer_{};
		std::size_t caret_{};
	};

	// -----------------------------------------------------------------
	// Caretaker — stores memento pointers; does not touch editor fields
	// -----------------------------------------------------------------
	class TextHistoryCaretaker
	{
	public:
		void push_checkpoint(std::unique_ptr<TextDocumentMemento> checkpoint) { stack_.push_back(std::move(checkpoint)); }

		[[nodiscard]] bool has_checkpoint() const noexcept { return !stack_.empty(); }

		std::unique_ptr<TextDocumentMemento> pop_checkpoint()
		{
			if (stack_.empty()) return nullptr;
			auto top = std::move(stack_.back());
			stack_.pop_back();
			return top;
		}

	private:
		std::vector<std::unique_ptr<TextDocumentMemento>> stack_;
	};

	// -----------------------------------------------------------------
	// Example 2 — Game-style checkpoint: HP + level, memento stays private
	// -----------------------------------------------------------------
	class HeroMemento
	{
		friend class GameHero;

	private:
		explicit HeroMemento(int hp, int level) : hp_(hp), level_(level) {}

		int hp_{};
		int level_{};
	};

	class GameHero
	{
	public:
		explicit GameHero(int hp = 100, int level = 1) : hp_(hp), level_(level) {}

		[[nodiscard]] int hp() const noexcept { return hp_; }

		[[nodiscard]] int level() const noexcept { return level_; }

		void take_damage(int amount)
		{
			hp_ -= amount;
			if (hp_ < 0) hp_ = 0;
		}

		void gain_level()
		{
			++level_;
			hp_ += 25;
		}

		[[nodiscard]] std::unique_ptr<HeroMemento> create_save() const
		{
			return std::unique_ptr<HeroMemento>(new HeroMemento(hp_, level_));
		}

		void load_save(const HeroMemento& save)
		{
			hp_ = save.hp_;
			level_ = save.level_;
		}

	private:
		int hp_{};
		int level_{};
	};

	// -----------------------------------------------------------------
	// Example 3 — “Wide” metadata only: balance sheet token with public label
	// -----------------------------------------------------------------
	class LedgerMemento
	{
		friend class AccountLedger;

	public:
		[[nodiscard]] int balance_cents() const noexcept { return balance_cents_; }

		[[nodiscard]] std::string_view note() const noexcept { return note_; }

	private:
		explicit LedgerMemento(int cents, std::string note) : balance_cents_(cents), note_(std::move(note)) {}

		int balance_cents_{};
		std::string note_{};
	};

	class AccountLedger
	{
	public:
		explicit AccountLedger(int opening_balance_cents) : balance_cents_(opening_balance_cents) {}

		[[nodiscard]] int balance_cents() const noexcept { return balance_cents_; }

		void deposit(int cents) { balance_cents_ += cents; }

		void withdraw(int cents)
		{
			if (cents > balance_cents_) throw std::runtime_error("insufficient funds");
			balance_cents_ -= cents;
		}

		[[nodiscard]] std::unique_ptr<LedgerMemento> bookmark(std::string note) const
		{
			return std::unique_ptr<LedgerMemento>(new LedgerMemento(balance_cents_, std::move(note)));
		}

		void restore(const LedgerMemento& snap) { balance_cents_ = snap.balance_cents_; }

	private:
		int balance_cents_{};
	};

} // namespace usage_examples::patterns::behavioral::memento_gof

namespace {

	using usage_examples::patterns::behavioral::memento_gof::AccountLedger;
	using usage_examples::patterns::behavioral::memento_gof::GameHero;
	using usage_examples::patterns::behavioral::memento_gof::TextDocument;
	using usage_examples::patterns::behavioral::memento_gof::TextDocumentMemento;
	using usage_examples::patterns::behavioral::memento_gof::TextHistoryCaretaker;

	TEST(MementoUsageExamples, TextDocumentRoundTripPreservesBufferAndCaret)
	{
		TextDocument doc;
		doc.insert("hel");
		doc.insert("lo");
		auto snap = doc.create_memento("after hello");
		EXPECT_EQ(snap->label(), "after hello");
		doc.insert("!!!");
		doc.set_caret(3);
		doc.backspace();
		EXPECT_NE(doc.text(), "hello");
		doc.restore(*snap);
		EXPECT_EQ(doc.text(), "hello");
		EXPECT_EQ(doc.caret(), 5u);
	}

	TEST(MementoUsageExamples, CaretakerStackSupportsCheckpointPop)
	{
		TextDocument doc;
		TextHistoryCaretaker history;
		doc.insert("v1");
		history.push_checkpoint(doc.create_memento("v1"));
		doc.insert("-delta");
		EXPECT_EQ(doc.text(), "v1-delta");
		auto mem = history.pop_checkpoint();
		ASSERT_NE(mem, nullptr);
		doc.restore(*mem);
		EXPECT_EQ(doc.text(), "v1");
	}

	TEST(MementoUsageExamples, HeroSaveAndLoadAfterCombat)
	{
		GameHero hero(120, 2);
		auto checkpoint = hero.create_save();
		hero.take_damage(80);
		hero.gain_level();
		EXPECT_LT(hero.hp(), 120);
		EXPECT_GE(hero.level(), 3);
		hero.load_save(*checkpoint);
		EXPECT_EQ(hero.hp(), 120);
		EXPECT_EQ(hero.level(), 2);
	}

	TEST(MementoUsageExamples, LedgerBookmarkExposesBalanceForUiButOriginatorRestores)
	{
		AccountLedger ledger(10'000);
		ledger.deposit(500);
		auto bookmark = ledger.bookmark("after payroll");
		EXPECT_EQ(bookmark->balance_cents(), 10'500);
		EXPECT_EQ(bookmark->note(), "after payroll");
		ledger.withdraw(9'000);
		EXPECT_EQ(ledger.balance_cents(), 1'500);
		ledger.restore(*bookmark);
		EXPECT_EQ(ledger.balance_cents(), 10'500);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Pair mementos with **Command**: push `create_memento()` in `execute()`,
 *    call `restore` inside `undo()` for a multi-level text editor.
 * 2. Add **schema_version** to a serialized memento and **migrate** old saves.
 * 3. Implement **copy-on-write** strings so frequent snapshots share storage
 *    until a mutation occurs.
 * 4. Store mementos in a **ring buffer** Caretaker with configurable depth and
 *    telemetry on evicted checkpoints.
 */
