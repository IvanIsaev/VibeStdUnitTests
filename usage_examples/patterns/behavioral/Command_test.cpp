/*
 * =============================================================================
 * Command (Gang of Four — Behavioral)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Encapsulate a **request** as an object, letting you **parameterize** clients
 * with different requests, **queue** or **log** operations, and support
 * **undoable** operations.
 *
 * The key move is: instead of the client calling `receiver.doThing()` directly,
 * it builds a **Command** object whose `execute()` forwards to the receiver.
 * That indirection buys you **decoupling**, **composition**, and **history**.
 *
 * Typical structure
 * -----------------
 *   • **Command** — declares `execute()` (and often `undo()`); may store
 *     arguments and a reference/pointer to the **Receiver**.
 *
 *   • **ConcreteCommand** — binds a receiver method + arguments; `execute()`
 *     calls `receiver.action(args)`.
 *
 *   • **Receiver** — knows how to perform the real work (`Document`, `Device`,
 *     `GameActor`, …).
 *
 *   • **Invoker** — holds a command (or queue); `invoke()` calls `execute()`
 *     without knowing the concrete operation (menu item, toolbar button, remote).
 *
 *   • **Client** — creates ConcreteCommand + wires receiver; passes command to
 *     invoker or history.
 *
 * What you gain
 * -------------
 *   • **Undo / redo** — store executed commands (or inverse operations /
 *     snapshots) and walk the stack.
 *
 *   • **Macro / composite** — one command runs a **sequence**; undo runs the
 *     sequence **backward**.
 *
 *   • **Deferred execution** — enqueue commands on a **job queue**, **network
 *     RPC**, or **transaction log**; replay later for crash recovery or testing.
 *
 *   • **Decouple UI from domain** — toolbar, CLI, script, and hotkey all build
 *     the same Command type; the document never depends on Qt vs web vs TUI.
 *
 * Command vs similar patterns
 * ---------------------------
 *   • **Strategy** — interchangeable **algorithms** inside one context; Command
 *     is a **named request** often with **history** and **queuing**.
 *
 *   • **Memento** — stores **snapshot** of receiver state; Command can *use*
 *     Memento for undo instead of inverse operations.
 *
 *   • **Chain of Responsibility** — passes a request along until handled;
 *     Command **targets** one receiver via encapsulation.
 *
 * Undo strategies (implementation notes)
 * --------------------------------------
 *   • **Inverse operations** — `execute()` inserts text, `undo()` deletes the
 *     same span (compact; must be correct under all interleavings).
 *
 *   • **Before/after snapshots** — copy minimal state before mutate; `undo()`
 *     restores (simple but memory-heavy for large buffers).
 *
 *   • **Command coalescing** — merge consecutive typed characters into one
 *     macro-command for fewer stack entries (common in editors).
 *
 * C++ implementation flavors
 * --------------------------
 *   • **Virtual `ICommand`** — classic OOP; easy polymorphic queues and undo
 *     stacks (`std::vector<std::unique_ptr<ICommand>>`).
 *
 *   • **`std::function<void()>`** — lightweight **closure** commands; great for
 *     glue code; **undo** needs a second functor or a small wrapper struct.
 *
 *   • **CRTP / templates** — zero-cost `execute` if you know receivers at
 *     compile time; less convenient for heterogeneous runtime menus.
 *
 *   • **Coroutines / async** — “command” becomes a resumable task; still pair
 *     with explicit cancellation / compensation for user-visible undo.
 *
 * Practical pitfalls
 * ------------------
 *   • **Lifetime** — commands must not outlive the **receiver** they capture
 *     unless you use `weak_ptr` / IDs and resolve targets at `execute()`.
 *
 *   • **Non-idempotent execute** — calling `execute()` twice by mistake should be
 *     defined (forbidden, or safe by design).
 *
 *   • **Undo after mutation** — after `undo()`, clear **redo** stack; after new
 *     `execute()`, truncate redo (standard editor semantics).
 *
 *   • **Threading** — queue drains on a worker; document **mutex** or **strand**
 *     must serialize `execute`/`undo` relative to other mutations.
 *
 *   • **One-definition rule** — this repo links many demos into
 *     `usage_examples`; demo types live in `command_gof` to avoid clashing with
 *     other files that reuse short names (`Circle`, `Document`, …).
 *
 * Testing ideas
 * -------------
 *   • **Golden receiver** — assert receiver state after each `execute`/`undo`.
 *
 *   • **Round-trip** — `execute(); undo();` leaves state unchanged.
 *
 *   • **Macro** — composite runs N steps; undo reverses all in order.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace usage_examples::patterns::behavioral::command_gof {

	// -----------------------------------------------------------------
	// Receiver — domain object that performs real work.
	// -----------------------------------------------------------------
	class TextScratchpad
	{
	public:
		[[nodiscard]] const std::string& text() const { return text_; }

		void clear() { text_.clear(); }

		void append(std::string_view chunk) { text_.append(chunk); }

		void remove_suffix(std::size_t byte_count)
		{
			if (byte_count > text_.size()) throw std::out_of_range("remove_suffix");
			text_.resize(text_.size() - byte_count);
		}

	private:
		std::string text_;
	};

	// -----------------------------------------------------------------
	// Command abstraction — execute + undo for reversible edits.
	// -----------------------------------------------------------------
	struct IEditCommand
	{
		virtual ~IEditCommand() = default;
		virtual void execute() = 0;
		virtual void undo() = 0;
	};

	// Appends text; undo removes exactly the bytes appended by this command.
	struct AppendTextCommand final : IEditCommand
	{
		TextScratchpad* doc{};
		std::string payload;

		AppendTextCommand(TextScratchpad* d, std::string p) : doc(d), payload(std::move(p)) {}

		void execute() override
		{
			if (!doc) throw std::logic_error("AppendTextCommand: null doc");
			doc->append(payload);
		}

		void undo() override
		{
			if (!doc) throw std::logic_error("AppendTextCommand: null doc");
			doc->remove_suffix(payload.size());
		}
	};

	// Deletes up to `max_chars` from the end; undo restores the removed suffix.
	struct DeleteSuffixCommand final : IEditCommand
	{
		TextScratchpad* doc{};
		std::size_t max_chars{};
		std::string removed_snapshot;

		DeleteSuffixCommand(TextScratchpad* d, std::size_t n) : doc(d), max_chars(n) {}

		void execute() override
		{
			if (!doc) throw std::logic_error("DeleteSuffixCommand: null doc");
			const auto& t = doc->text();
			const std::size_t take = (std::min)(max_chars, t.size());
			removed_snapshot = t.substr(t.size() - take);
			doc->remove_suffix(take);
		}

		void undo() override
		{
			if (!doc) throw std::logic_error("DeleteSuffixCommand: null doc");
			doc->append(removed_snapshot);
		}
	};

	// Runs child commands in order; undo runs children in reverse order.
	struct MacroCommand final : IEditCommand
	{
		std::vector<std::unique_ptr<IEditCommand>> steps;

		void add_step(std::unique_ptr<IEditCommand> cmd) { steps.push_back(std::move(cmd)); }

		void execute() override
		{
			for (auto& s : steps) s->execute();
		}

		void undo() override
		{
			for (auto it = steps.rbegin(); it != steps.rend(); ++it) (*it)->undo();
		}
	};

	// -----------------------------------------------------------------
	// Invoker — applies commands and maintains undo / redo stacks.
	// -----------------------------------------------------------------
	class EditHistory
	{
	public:
		explicit EditHistory(TextScratchpad* document) : doc_(document) {}

		void run(std::unique_ptr<IEditCommand> cmd)
		{
			cmd->execute();
			undo_stack_.push_back(std::move(cmd));
			redo_stack_.clear();
		}

		void undo()
		{
			if (undo_stack_.empty()) return;
			auto cmd = std::move(undo_stack_.back());
			undo_stack_.pop_back();
			cmd->undo();
			redo_stack_.push_back(std::move(cmd));
		}

		void redo()
		{
			if (redo_stack_.empty()) return;
			auto cmd = std::move(redo_stack_.back());
			redo_stack_.pop_back();
			cmd->execute();
			undo_stack_.push_back(std::move(cmd));
		}

		[[nodiscard]] std::size_t undo_depth() const { return undo_stack_.size(); }

		[[nodiscard]] std::size_t redo_depth() const { return redo_stack_.size(); }

	private:
		TextScratchpad* doc_{};
		std::vector<std::unique_ptr<IEditCommand>> undo_stack_;
		std::vector<std::unique_ptr<IEditCommand>> redo_stack_;
	};

} // namespace usage_examples::patterns::behavioral::command_gof

namespace {

	using usage_examples::patterns::behavioral::command_gof::AppendTextCommand;
	using usage_examples::patterns::behavioral::command_gof::DeleteSuffixCommand;
	using usage_examples::patterns::behavioral::command_gof::EditHistory;
	using usage_examples::patterns::behavioral::command_gof::MacroCommand;
	using usage_examples::patterns::behavioral::command_gof::TextScratchpad;

	TEST(CommandUsageExamples, AppendAndUndoRoundTrip)
	{
		TextScratchpad doc;
		auto cmd = std::make_unique<AppendTextCommand>(&doc, "hello");
		cmd->execute();
		EXPECT_EQ(doc.text(), "hello");
		cmd->undo();
		EXPECT_TRUE(doc.text().empty());
	}

	TEST(CommandUsageExamples, InvokerTracksUndoRedo)
	{
		TextScratchpad doc;
		EditHistory history(&doc);
		history.run(std::make_unique<AppendTextCommand>(&doc, "a"));
		history.run(std::make_unique<AppendTextCommand>(&doc, "b"));
		EXPECT_EQ(doc.text(), "ab");
		EXPECT_EQ(history.undo_depth(), 2u);
		history.undo();
		EXPECT_EQ(doc.text(), "a");
		EXPECT_EQ(history.redo_depth(), 1u);
		history.redo();
		EXPECT_EQ(doc.text(), "ab");
	}

	TEST(CommandUsageExamples, NewCommandClearsRedoStack)
	{
		TextScratchpad doc;
		EditHistory history(&doc);
		history.run(std::make_unique<AppendTextCommand>(&doc, "x"));
		history.undo();
		EXPECT_EQ(history.redo_depth(), 1u);
		history.run(std::make_unique<AppendTextCommand>(&doc, "y"));
		EXPECT_EQ(doc.text(), "y");
		EXPECT_EQ(history.redo_depth(), 0u);
	}

	TEST(CommandUsageExamples, DeleteSuffixRestoresOnUndo)
	{
		TextScratchpad doc;
		doc.append("abcdef");
		auto del = std::make_unique<DeleteSuffixCommand>(&doc, 3);
		del->execute();
		EXPECT_EQ(doc.text(), "abc");
		del->undo();
		EXPECT_EQ(doc.text(), "abcdef");
	}

	TEST(CommandUsageExamples, MacroRunsStepsAndUndoesInReverse)
	{
		TextScratchpad doc;
		auto macro = std::make_unique<MacroCommand>();
		macro->add_step(std::make_unique<AppendTextCommand>(&doc, "["));
		macro->add_step(std::make_unique<AppendTextCommand>(&doc, "note"));
		macro->add_step(std::make_unique<AppendTextCommand>(&doc, "]"));
		macro->execute();
		EXPECT_EQ(doc.text(), "[note]");
		macro->undo();
		EXPECT_TRUE(doc.text().empty());
	}

	TEST(CommandUsageExamples, FunctionalStyleClosureCommand)
	{
		// Closure-based “command” without a class per operation: pair do/undo.
		TextScratchpad doc;
		std::string stash;
		auto execute_fn = [&] { stash = doc.text(); doc.append("!"); };
		auto undo_fn = [&] {
			doc.clear();
			doc.append(stash);
		};
		execute_fn();
		EXPECT_EQ(doc.text(), "!");
		undo_fn();
		EXPECT_TRUE(doc.text().empty());
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add **coalescing**: merge adjacent `AppendTextCommand` with single-char
 *    payloads into one undo step while typing.
 * 2. Persist a **command log** to disk and **replay** after restart (audit trail).
 * 3. Combine with **Memento**: store full document snapshots every K commands for
 *    crash recovery; undo walks back to nearest snapshot then reapplies commands.
 * 4. Wrap `IEditCommand` in a **transaction** that rolls back all steps if any
 *    `execute()` throws halfway through a macro.
 */
