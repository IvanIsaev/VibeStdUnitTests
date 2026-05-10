/*
 * =============================================================================
 * Iterator (Gang of Four — Behavioral)
 * =============================================================================
 *
 * Intent (GoF)
 * ------------
 * Provide a way to access the elements of an **aggregate object** sequentially
 * **without exposing** its underlying representation (array, tree, linked list,
 * database cursor, …).
 *
 * The Iterator pattern separates **traversal** from the **collection** so you
 * can have multiple traversals in flight, hide index arithmetic, and swap data
 * structures behind the same traversal API.
 *
 * Classic participants
 * --------------------
 *   • **Iterator** — defines the traversal interface (`first`, `next`,
 *     `is_done`, `current_item` in the GoF book; modern C++ often uses
 *     `begin`/`end`, `operator++`, `operator*`).
 *
 *   • **ConcreteIterator** — implements the traversal for a specific aggregate;
 *     tracks **current position** (index, node pointer, cursor handle).
 *
 *   • **Aggregate** — factory for iterators (`create_iterator`) and usually a
 *     way to **count** or **bound** traversal.
 *
 *   • **ConcreteAggregate** — returns a matching ConcreteIterator over its
 *     storage.
 *
 * Internal vs external iterators
 * ------------------------------
 *   • **External iterator** — client drives the loop (`for (; !it.is_done();
 *     it.next()) use(it.current())`). Most C++ library iterators are external.
 *
 *   • **Internal iterator** — the aggregate accepts a **callback** / visitor
 *     and walks itself (`for_each(lambda)`). Simpler call sites; harder to
 *     `break` mid-traversal or interleave two walks without cooperative APIs.
 *
 * Iterators in C++ (STL and beyond)
 * ---------------------------------
 * The standard library **is** the Iterator pattern taken seriously:
 * `std::vector::iterator`, `std::map::const_iterator`, stream iterators, etc.
 * Algorithms (`std::for_each`, `std::ranges::filter`) are written against
 * iterator pairs or ranges, not concrete containers.
 *
 * C++17 **`range-for`** desugars to `begin`/`end` calls. C++20 **ranges** add
 * laziness (`views::filter`, `views::transform`) — still iterator-shaped under
 * the hood.
 *
 * **Do not confuse** this pattern’s name with the deprecated `std::iterator`
 * base class; prefer explicitly defining iterator traits or inheriting from
 * `std::iterator_traits` specializations when you need compatibility.
 *
 * Design forces
 * -------------
 *   • **Robustness** — define behavior when the **aggregate mutates** during
 *     iteration (reallocation, iterator invalidation rules for `vector`, …).
 *
 *   • **Const correctness** — provide `const_iterator` / `ConstIterator` paths
 *     that cannot mutate elements through the iterator.
 *
 *   • **Multiple active iterators** — trees and lists usually allow many; some
 *     generators are **single-pass** (`std::istream_iterator`).
 *
 *   • **Bidirectional / random access** — richer categories enable more
 *     algorithms but complicate the aggregate’s abstraction.
 *
 * Related patterns
 * ----------------
 *   • **Composite** — often traversed with iterators or visitors over the tree.
 *
 *   • **Factory Method** — `Aggregate::create_iterator()` is a small factory.
 *
 *   • **Memento** — store iterator position for undo / bookmarking.
 *
 *   • **Visitor** — iteration visits nodes; Visitor performs per-node actions.
 *
 * Testing
 * -------
 *   • Walk known fixtures; compare **element sequences** to golden vectors.
 *
 *   • **Empty**, **single**, **large** aggregates; **const** vs mutable paths.
 *
 *   • If iterators compare by position, test **equality** at boundaries.
 *
 *   • **One-definition rule** — demo types live in `iterator_gof` alongside
 *     other `usage_examples` translation units.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace usage_examples::patterns::behavioral::iterator_gof {

	// -----------------------------------------------------------------
	// Example 1 — “Textbook” GoF cursor over a hidden vector<int>
	// -----------------------------------------------------------------
	class IntegerSequenceBag
	{
	public:
		explicit IntegerSequenceBag(std::vector<int> values) : values_(std::move(values)) {}

		[[nodiscard]] std::size_t count() const { return values_.size(); }

		class ClassicForwardCursor
		{
		public:
			explicit ClassicForwardCursor(const IntegerSequenceBag* owner) : owner_(owner) {}

			void first() { index_ = 0; }

			void next()
			{
				if (is_done()) throw std::out_of_range("ClassicForwardCursor::next");
				++index_;
			}

			[[nodiscard]] bool is_done() const { return !owner_ || index_ >= owner_->count(); }

			[[nodiscard]] int current_item() const
			{
				if (is_done()) throw std::out_of_range("ClassicForwardCursor::current_item");
				return owner_->value_at(index_);
			}

		private:
			const IntegerSequenceBag* owner_{};
			std::size_t index_{};
		};

		[[nodiscard]] ClassicForwardCursor create_iterator() const { return ClassicForwardCursor(this); }

		[[nodiscard]] int value_at(std::size_t i) const { return values_.at(i); }

	private:
		std::vector<int> values_;
	};

	// -----------------------------------------------------------------
	// Example 2 — C++ forward iterators + range-for on a small playlist
	// -----------------------------------------------------------------
	class Playlist
	{
	public:
		void add_track(std::string title) { tracks_.push_back(std::move(title)); }

		class Iterator
		{
		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = std::string;
			using pointer = std::string*;
			using reference = std::string&;

			Iterator() = default;

			reference operator*() const { return *it_; }

			Iterator& operator++()
			{
				++it_;
				return *this;
			}

			Iterator operator++(int)
			{
				Iterator tmp = *this;
				++(*this);
				return tmp;
			}

			friend bool operator==(const Iterator& a, const Iterator& b) { return a.it_ == b.it_; }

		private:
			friend class Playlist;
			explicit Iterator(std::vector<std::string>::iterator i) : it_(std::move(i)) {}

			std::vector<std::string>::iterator it_{};
		};

		class ConstIterator
		{
		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = const std::string;
			using pointer = const std::string*;
			using reference = const std::string&;

			ConstIterator() = default;

			reference operator*() const { return *it_; }

			ConstIterator& operator++()
			{
				++it_;
				return *this;
			}

			friend bool operator==(const ConstIterator& a, const ConstIterator& b) { return a.it_ == b.it_; }

		private:
			friend class Playlist;
			explicit ConstIterator(std::vector<std::string>::const_iterator i) : it_(std::move(i)) {}

			std::vector<std::string>::const_iterator it_{};
		};

		[[nodiscard]] Iterator begin() { return Iterator(tracks_.begin()); }

		[[nodiscard]] Iterator end() { return Iterator(tracks_.end()); }

		[[nodiscard]] ConstIterator begin() const { return ConstIterator(tracks_.begin()); }

		[[nodiscard]] ConstIterator end() const { return ConstIterator(tracks_.end()); }

	private:
		std::vector<std::string> tracks_;
	};

	inline bool operator!=(const Playlist::Iterator& a, const Playlist::Iterator& b) { return !(a == b); }

	inline bool operator!=(const Playlist::ConstIterator& a, const Playlist::ConstIterator& b) { return !(a == b); }

	// -----------------------------------------------------------------
	// Example 3 — External iterator that *skips* odd numbers (view behavior)
	// -----------------------------------------------------------------
	class EvenIntegersView
	{
	public:
		explicit EvenIntegersView(const std::vector<int>& source) : source_(&source) {}

		class Iterator
		{
		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = int;
			using pointer = const int*;
			using reference = const int&;

			Iterator() = default;

			reference operator*() const { return (*source_)[index_]; }

			Iterator& operator++()
			{
				advance_past_non_even();
				return *this;
			}

			friend bool operator==(const Iterator& a, const Iterator& b)
			{
				return a.source_ == b.source_ && a.index_ == b.index_;
			}

		private:
			friend class EvenIntegersView;
			const std::vector<int>* source_{};
			std::size_t index_{};

			Iterator(const std::vector<int>* src, std::size_t start) : source_(src), index_(start)
			{
				skip_to_even();
			}

			void skip_to_even()
			{
				while (source_ && index_ < source_->size() && ((*source_)[index_] & 1) != 0) ++index_;
			}

			void advance_past_non_even()
			{
				if (!source_ || index_ >= source_->size()) return;
				++index_;
				skip_to_even();
			}
		};

		[[nodiscard]] Iterator begin() const { return Iterator(source_, 0); }

		[[nodiscard]] Iterator end() const { return Iterator(source_, source_->size()); }

	private:
		const std::vector<int>* source_{};
	};

	inline bool operator!=(const EvenIntegersView::Iterator& a, const EvenIntegersView::Iterator& b) { return !(a == b); }

	// -----------------------------------------------------------------
	// Example 4 — Binary tree preorder traversal without recursion (stack)
	// -----------------------------------------------------------------
	struct BinaryTreeNode
	{
		std::string key;
		std::unique_ptr<BinaryTreeNode> left;
		std::unique_ptr<BinaryTreeNode> right;
	};

	class BinaryPreorderIterator
	{
	public:
		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = BinaryTreeNode;
		using pointer = BinaryTreeNode*;
		using reference = BinaryTreeNode&;

		BinaryPreorderIterator() = default;

		explicit BinaryPreorderIterator(BinaryTreeNode* root)
		{
			if (root) stack_.push_back(root);
		}

		reference operator*() const { return *stack_.back(); }

		pointer operator->() const { return stack_.back(); }

		BinaryPreorderIterator& operator++()
		{
			if (stack_.empty()) return *this;
			BinaryTreeNode* cur = stack_.back();
			stack_.pop_back();
			if (cur->right) stack_.push_back(cur->right.get());
			if (cur->left) stack_.push_back(cur->left.get());
			return *this;
		}

		friend bool operator==(const BinaryPreorderIterator& a, const BinaryPreorderIterator& b)
		{
			return a.stack_ == b.stack_;
		}

	private:
		std::vector<BinaryTreeNode*> stack_;
	};

	inline bool operator!=(const BinaryPreorderIterator& a, const BinaryPreorderIterator& b) { return !(a == b); }

} // namespace usage_examples::patterns::behavioral::iterator_gof

namespace {

	using usage_examples::patterns::behavioral::iterator_gof::BinaryPreorderIterator;
	using usage_examples::patterns::behavioral::iterator_gof::BinaryTreeNode;
	using usage_examples::patterns::behavioral::iterator_gof::EvenIntegersView;
	using usage_examples::patterns::behavioral::iterator_gof::IntegerSequenceBag;
	using usage_examples::patterns::behavioral::iterator_gof::Playlist;

	TEST(IteratorUsageExamples, ClassicGoFCursorWalksAggregate)
	{
		const IntegerSequenceBag bag({ 10, 20, 30 });
		auto cursor = bag.create_iterator();
		cursor.first();
		std::vector<int> out;
		for (; !cursor.is_done(); cursor.next()) out.push_back(cursor.current_item());
		EXPECT_EQ(out, (std::vector<int>{ 10, 20, 30 }));
		EXPECT_EQ(bag.count(), 3u);
	}

	TEST(IteratorUsageExamples, PlaylistSupportsRangeBasedFor)
	{
		Playlist pl;
		pl.add_track("foo");
		pl.add_track("bar");
		std::string acc;
		for (const auto& t : pl) acc += t;
		EXPECT_EQ(acc, "foobar");
	}

	TEST(IteratorUsageExamples, EvenIntegersViewSkipsOddValues)
	{
		const std::vector<int> src{ 1, 2, 3, 4, 5, 6, 7 };
		std::vector<int> out;
		for (int v : EvenIntegersView(src)) out.push_back(v);
		EXPECT_EQ(out, (std::vector<int>{ 2, 4, 6 }));
	}

	TEST(IteratorUsageExamples, BinaryTreePreorderIteratorMatchesManualStackWalk)
	{
		auto root = std::make_unique<BinaryTreeNode>();
		root->key = "A";
		root->left = std::make_unique<BinaryTreeNode>();
		root->left->key = "B";
		root->right = std::make_unique<BinaryTreeNode>();
		root->right->key = "C";
		root->left->left = std::make_unique<BinaryTreeNode>();
		root->left->left->key = "D";

		std::vector<std::string> keys;
		for (BinaryPreorderIterator it(root.get()), end; it != end; ++it) keys.push_back(it->key);
		EXPECT_EQ(keys, (std::vector<std::string>{ "A", "B", "D", "C" }));
	}

	TEST(IteratorUsageExamples, MutablePlaylistIteratorCanRenameTrack)
	{
		Playlist pl;
		pl.add_track("old");
		for (auto& t : pl) t = "new";
		EXPECT_EQ(*pl.begin(), "new");
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add **bidirectional** iterators on `Playlist` (reverse iteration over the
 *    underlying vector’s reverse iterators).
 * 2. Implement **level-order** (BFS) iterators for `BinaryTreeNode` using a
 *    `std::queue` instead of a stack.
 * 3. Wrap a **database cursor** or **file line reader** in an input iterator
 *    with single-pass semantics and explicit `at_end()`.
 * 4. Compare this manual tree iterator with **C++20 coroutine** generators
 *    (`generator<std::string>`) for preorder — discuss allocation and laziness.
 */
