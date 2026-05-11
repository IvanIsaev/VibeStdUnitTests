/*
 * =============================================================================
 * Producer–Consumer (Concurrency pattern)
 * =============================================================================
 *
 * What it is
 * ----------
 * **Producers** generate **items** (tasks, messages, byte chunks) and **push**
 * them into a **buffer**. **Consumers** **pop** items and process them. The
 * buffer **decouples** production rate from consumption rate so bursts on either
 * side do not force both ends to move in lockstep.
 *
 * This is the structural heart of **message queues**, **thread pools** (tasks
 * are produced into a work queue), **pipelines**, and many **event loops** that
 * drain a backlog on a dedicated thread.
 *
 * Core design choices
 * -------------------
 *   • **Bounded vs unbounded buffer**
 *
 *       – **Bounded** — `push` may **block** (or fail) when full → natural
 *         **back-pressure**; protects memory under a flood of producers.
 *
 *       – **Unbounded** — `push` rarely blocks; risk of **OOM** if consumers
 *         stall; simplest API for “never drop” prototypes.
 *
 *   • **Blocking vs lock-free** — `std::mutex` + `std::condition_variable` is
 *     the textbook monitor implementation; **MPMC ring buffers** trade
 *     complexity for latency at high throughput.
 *
 *   • **Fan-out / fan-in** — one producer / many consumers (work stealing),
 *     many producers / one consumer (aggregation), or **N:M** with a shared
 *     queue (needs careful **shutdown**).
 *
 * Coordination primitives
 * -----------------------
 *   • **Two condition variables** on a bounded queue — `not_full` for
 *     producers, `not_empty` for consumers (Dijkstra-style **bounded buffer**).
 *
 *   • **Semaphores** — count free slots vs count items; equivalent expressive
 *     power with different ergonomics (`std::counting_semaphore` in C++20).
 *
 *   • **Poison pills / close token** — push a **sentinel** value (or a dedicated
 *     “channel closed” state) so each consumer exits cleanly; avoids threads
 *     blocked forever after shutdown.
 *
 * Pitfalls
 * --------
 *   • **Lost wakeups** — always mutate queue state and **predicate** under the
 *     same mutex you use with `condition_variable`.
 *
 *   • **Deadlock** — multiple locks (queue + downstream resource); define a
 *     **global lock order** or use **single** queue mutex for the buffer.
 *
 *   • **Fairness** — `notify_one` may **starve** some waiters under pathological
 *     scheduling; `notify_all` reduces starvation but increases **thundering
 *     herd** cost.
 *
 *   • **Exception safety** — if consumer throws, decide whether to **requeue**,
 *     **log and drop**, or **crash**; producers may still be running.
 *
 * Related patterns
 * ----------------
 *   • **Monitor Object** — the buffer is often a monitor (this file’s bounded
 *     queue is a monitor).
 *
 *   • **Thread Pool** — consumers are **fixed** worker threads; producers
 *     **submit** tasks.
 *
 *   • **Active Object** — sometimes consumes from a **private** mailbox instead
 *     of a shared global queue.
 *
 * Testing
 * -------
 *   • **Order / totals** — checksums or sorted replay for deterministic payloads.
 *
 *   • **Shutdown** — producers finish, sentinels delivered, all consumers join.
 *
 *   • **Stress** — many producers under TSan; assert **no lost items** when
 *     using a counted workload.
 *
 *   • **One-definition rule** — types live in `producer_consumer_demo` for this
 *     `usage_examples` binary.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

namespace usage_examples::patterns::concurrency::producer_consumer_demo {

	// Sentinel: producers must not send this value as data (tests use small positives only).
	inline constexpr int kStopToken = (std::numeric_limits<int>::min)();

	// -----------------------------------------------------------------
	// Bounded buffer — producers block when full, consumers when empty
	// -----------------------------------------------------------------
	class BoundedIntChannel
	{
	public:
		explicit BoundedIntChannel(std::size_t capacity) : capacity_(capacity)
		{
			if (capacity_ == 0) throw std::invalid_argument("BoundedIntChannel: capacity");
		}

		void push(int value)
		{
			std::unique_lock lk(mx_);
			not_full_.wait(lk, [this] { return closed_ || queue_.size() < capacity_; });
			if (closed_) throw std::logic_error("BoundedIntChannel: push after close");
			queue_.push(value);
			not_empty_.notify_one();
		}

		int take()
		{
			std::unique_lock lk(mx_);
			not_empty_.wait(lk, [this] { return closed_ || !queue_.empty(); });
			if (queue_.empty() && closed_) throw std::logic_error("BoundedIntChannel: take on drained closed channel");
			const int v = queue_.front();
			queue_.pop();
			not_full_.notify_one();
			return v;
		}

		void close_no_more_pushes()
		{
			{
				std::lock_guard lk(mx_);
				closed_ = true;
			}
			not_full_.notify_all();
			not_empty_.notify_all();
		}

		[[nodiscard]] bool is_closed() const
		{
			std::lock_guard lk(mx_);
			return closed_;
		}

		void push_stop_tokens_for_consumers(int consumer_count)
		{
			for (int i = 0; i < consumer_count; ++i) push(kStopToken);
		}

	private:
		mutable std::mutex mx_{};
		std::condition_variable not_full_{};
		std::condition_variable not_empty_{};
		std::queue<int> queue_{};
		std::size_t capacity_{};
		bool closed_{false};
	};

	// -----------------------------------------------------------------
	// Unbounded buffer — push never waits for space; close wakes waiters
	// -----------------------------------------------------------------
	class UnboundedIntChannel
	{
	public:
		void push(int value)
		{
			{
				std::lock_guard lk(mx_);
				if (closed_) throw std::logic_error("UnboundedIntChannel: push after close");
				queue_.push(value);
			}
			not_empty_.notify_one();
		}

		std::optional<int> take()
		{
			std::unique_lock lk(mx_);
			not_empty_.wait(lk, [this] { return closed_ || !queue_.empty(); });
			if (queue_.empty()) return std::nullopt;
			const int v = queue_.front();
			queue_.pop();
			return v;
		}

		void close()
		{
			{
				std::lock_guard lk(mx_);
				closed_ = true;
			}
			not_empty_.notify_all();
		}

	private:
		std::mutex mx_{};
		std::condition_variable not_empty_{};
		std::queue<int> queue_{};
		bool closed_{false};
	};

} // namespace usage_examples::patterns::concurrency::producer_consumer_demo

namespace {

	using usage_examples::patterns::concurrency::producer_consumer_demo::BoundedIntChannel;
	using usage_examples::patterns::concurrency::producer_consumer_demo::kStopToken;
	using usage_examples::patterns::concurrency::producer_consumer_demo::UnboundedIntChannel;

	TEST(ProducerConsumerUsageExamples, SingleProducerSingleConsumerPreservesOrder)
	{
		BoundedIntChannel ch(4);
		std::vector<int> out;
		std::thread consumer([&] {
			for (int i = 0; i < 3; ++i) out.push_back(ch.take());
		});
		ch.push(10);
		ch.push(20);
		ch.push(30);
		consumer.join();
		EXPECT_EQ(out, (std::vector<int>{ 10, 20, 30 }));
	}

	TEST(ProducerConsumerUsageExamples, BoundedChannelBackPressuresProducer)
	{
		BoundedIntChannel ch(2);
		ch.push(1);
		ch.push(2);
		bool third = false;
		std::thread slow([&] {
			ch.push(3);
			third = true;
		});
		std::this_thread::sleep_for(std::chrono::milliseconds(15));
		EXPECT_FALSE(third);
		EXPECT_EQ(ch.take(), 1);
		slow.join();
		EXPECT_TRUE(third);
		EXPECT_EQ(ch.take(), 2);
		EXPECT_EQ(ch.take(), 3);
	}

	TEST(ProducerConsumerUsageExamples, MultipleConsumersDrainWithStopTokens)
	{
		BoundedIntChannel ch(8);
		constexpr int kItems = 120;
		constexpr int kConsumers = 4;

		std::thread producer([&] {
			for (int i = 1; i <= kItems; ++i) ch.push(i);
			ch.push_stop_tokens_for_consumers(kConsumers);
		});

		std::atomic<long long> sum{0};
		std::vector<std::thread> consumers;
		consumers.reserve(kConsumers);
		for (int c = 0; c < kConsumers; ++c)
		{
			(void)c;
			consumers.emplace_back([&] {
				for (;;)
				{
					const int v = ch.take();
					if (v == kStopToken) break;
					sum.fetch_add(v, std::memory_order_relaxed);
				}
			});
		}

		producer.join();
		for (auto& t : consumers) t.join();

		const long long expected = static_cast<long long>(kItems) * (kItems + 1) / 2;
		EXPECT_EQ(sum.load(), expected);
	}

	TEST(ProducerConsumerUsageExamples, FanInManyProducersOneConsumer)
	{
		BoundedIntChannel ch(16);
		constexpr int kProducers = 6;
		constexpr int kEach = 50;
		std::vector<std::thread> producers;
		producers.reserve(kProducers);
		for (int p = 0; p < kProducers; ++p)
		{
			producers.emplace_back([&ch, p] {
				for (int i = 0; i < kEach; ++i) ch.push(p * kEach + i);
			});
		}

		long long acc = 0;
		for (int n = 0; n < kProducers * kEach; ++n) acc += ch.take();

		for (auto& t : producers) t.join();

		const long long last = static_cast<long long>(kProducers * kEach - 1);
		const long long expected = last * (last + 1) / 2;
		EXPECT_EQ(acc, expected);
	}

	TEST(ProducerConsumerUsageExamples, UnboundedChannelDrainsAfterClose)
	{
		UnboundedIntChannel ch;
		ch.push(2);
		ch.push(3);
		ch.close();
		std::optional<int> a = ch.take();
		std::optional<int> b = ch.take();
		std::optional<int> c = ch.take();
		ASSERT_TRUE(a.has_value());
		ASSERT_TRUE(b.has_value());
		EXPECT_EQ(*a, 2);
		EXPECT_EQ(*b, 3);
		EXPECT_FALSE(c.has_value());
	}

	TEST(ProducerConsumerUsageExamples, ZeroCapacityThrows)
	{
		EXPECT_THROW(static_cast<void>(BoundedIntChannel(0)), std::invalid_argument);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add **try_push** / **try_take** with `std::chrono::milliseconds` and return
 *    `bool` / `std::optional<int>`.
 * 2. Implement a **lock-free SPSC** ring buffer and compare latency to the
 *    monitor-based bounded queue under one producer and one consumer.
 * 3. Layer **metrics** (high watermark, average queue depth) for capacity
 *    planning.
 * 4. Replace int payloads with **`std::function<void()>`** work items to build a
 *    minimal thread pool on top of the channel.
 */
