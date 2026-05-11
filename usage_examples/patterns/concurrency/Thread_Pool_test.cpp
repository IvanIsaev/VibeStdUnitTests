/*
 * =============================================================================
 * Thread Pool (Concurrency pattern)
 * =============================================================================
 *
 * What it is
 * ----------
 * A **thread pool** keeps a **fixed (or bounded) set of worker threads** alive
 * and feeds them **tasks** from a queue (or per-thread deques in work-stealing
 * designs). Callers **submit** work instead of spawning a fresh `std::thread`
 * per job, amortizing **OS thread creation**, **stack allocation**, and
 * **scheduler churn**.
 *
 * Typical ingredients
 * -------------------
 *   • **Worker threads** — long-lived loops that wait on a **condition variable**
 *     or **semaphore**, dequeue the next task, execute, repeat.
 *
 *   • **Task queue** — FIFO `std::queue` + mutex; or **lock-free** MPMC; or
 *     **work-stealing** deques (Chase–Lev) for fork/join parallelism.
 *
 *   • **Submission API** — `void post(std::function<void()>)` and/or
 *     `std::future` returning `submit` for results and exceptions.
 *
 *   • **Lifecycle** — **graceful shutdown**: stop accepting new work, wake
 *     workers, **drain** remaining tasks, **join** threads (or `jthread` with
 *     `stop_token`).
 *
 * Why use it
 * ----------
 *   • **Throughput** under bursty workloads — reuse threads instead of paying
 *     kernel transitions per micro-task.
 *
 *   • **Back-pressure** — bounded queues let you **reject**, **block**, or
 *     **shed load** when producers outrun consumers.
 *
 *   • **Resource caps** — upper bound on concurrent threads protects memory and
 *     keeps CPU usage predictable on small hosts.
 *
 * Design variants
 * ---------------
 *   • **Single global pool** vs **domain pools** (I/O pool, CPU pool) to avoid
 *     blocking tasks starving latency-sensitive work.
 *
 *   • **`std::async` / lazy futures** — convenient but implementation-defined
 *     threading; not a substitute for an explicit pool when you need **policy**.
 *
 *   • **Boost.Asio / Intel TBB / CUDA** — higher-level schedulers; same pattern
 *     at different scales.
 *
 * Pitfalls
 * --------
 *   • **Task explosion** — unbounded queues can **OOM**; bound them or use
 *     **drop** / **caller blocks** policies.
 *
 *   • **Deadlock** — pool tasks that **synchronously wait** on futures from the
 *     **same** pool can **starve** if all workers block (classic thread-pool
 *     deadlock). Offload waiting to another pool, use **async** continuations, or
 *     **reserve** threads.
 *
 *   • **Exception paths** — decide whether a throwing task **aborts the worker**
 *     (bad) vs **propagates via `future`** (good) vs **logs and continues**.
 *
 *   • **Destruction order** — joining workers **before** releasing resources
 *     tasks might touch; signal **stop** under the same mutex that guards the
 *     queue to avoid **use-after-free** on shutdown.
 *
 * Related patterns
 * ----------------
 *   • **Active Object** — pool of size 1 with strict ordering; thread pool is
 *     **many workers**, **unordered** execution unless you add **strands**.
 *
 *   • **Monitor Object** — the task queue is often a **monitor** (mutex + cv).
 *
 *   • **Object Pool** — pools **threads** instead of **heavy objects**.
 *
 * Testing
 * -------
 *   • **Determinism** — use barriers / `std::atomic` counters; avoid assuming
 *     task order unless the API guarantees FIFO completion.
 *
 *   • **Shutdown** — submit work, destroy pool, assert counters / no hangs.
 *
 *   • **Sanitizers** — TSan stress on `submit` from many producers.
 *
 *   • **One-definition rule** — types live in `thread_pool_demo` for this
 *     `usage_examples` binary.
 *
 * =============================================================================
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace usage_examples::patterns::concurrency::thread_pool_demo {

	class FixedThreadPool
	{
	public:
		explicit FixedThreadPool(std::size_t worker_count)
		{
			if (worker_count == 0) throw std::invalid_argument("FixedThreadPool: worker_count");
			workers_.reserve(worker_count);
			for (std::size_t i = 0; i < worker_count; ++i)
			{
				workers_.emplace_back([this] { worker_loop(); });
			}
		}

		FixedThreadPool(const FixedThreadPool&) = delete;
		FixedThreadPool& operator=(const FixedThreadPool&) = delete;

		~FixedThreadPool()
		{
			{
				std::lock_guard lk(mx_);
				stopped_ = true;
			}
			cv_.notify_all();
			for (std::thread& w : workers_)
			{
				if (w.joinable()) w.join();
			}
		}

		void submit_void(std::function<void()> job)
		{
			{
				std::lock_guard lk(mx_);
				if (stopped_) throw std::logic_error("FixedThreadPool: submit after shutdown");
				tasks_.push(std::move(job));
			}
			cv_.notify_one();
		}

		template <class F, class = std::enable_if_t<std::is_invocable_v<std::decay_t<F>>>>
		auto submit(F&& f) -> std::future<std::invoke_result_t<std::decay_t<F>>>
		{
			using R = std::invoke_result_t<std::decay_t<F>>;
			auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
			std::future<R> fut = task->get_future();
			submit_void([task]() { (*task)(); });
			return fut;
		}

		[[nodiscard]] std::size_t worker_count() const noexcept { return workers_.size(); }

	private:
		void worker_loop()
		{
			for (;;)
			{
				std::function<void()> job;
				{
					std::unique_lock lk(mx_);
					cv_.wait(lk, [this] { return stopped_ || !tasks_.empty(); });
					if (stopped_ && tasks_.empty()) return;
					job = std::move(tasks_.front());
					tasks_.pop();
				}
				job();
			}
		}

		std::mutex mx_{};
		std::condition_variable cv_{};
		std::queue<std::function<void()>> tasks_{};
		bool stopped_{false};
		std::vector<std::thread> workers_{};
	};

} // namespace usage_examples::patterns::concurrency::thread_pool_demo

namespace {

	using usage_examples::patterns::concurrency::thread_pool_demo::FixedThreadPool;

	TEST(ThreadPoolUsageExamples, SubmitVoidRunsTasksToCompletionBeforeDestructor)
	{
		std::atomic<int> counter{0};
		{
			FixedThreadPool pool(4);
			for (int i = 0; i < 100; ++i)
			{
				pool.submit_void([&counter] { ++counter; });
			}
		}
		EXPECT_EQ(counter.load(), 100);
	}

	TEST(ThreadPoolUsageExamples, SubmitReturnsFutureWithResult)
	{
		FixedThreadPool pool(2);
		std::future<int> fut = pool.submit([] { return 6 * 7; });
		ASSERT_EQ(fut.wait_for(std::chrono::seconds(1)), std::future_status::ready);
		EXPECT_EQ(fut.get(), 42);
	}

	TEST(ThreadPoolUsageExamples, ExceptionsPropagateThroughFuture)
	{
		FixedThreadPool pool(2);
		std::future<void> fut = pool.submit([] { throw std::runtime_error("boom"); });
		ASSERT_EQ(fut.wait_for(std::chrono::seconds(1)), std::future_status::ready);
		EXPECT_THROW(fut.get(), std::runtime_error);
	}

	TEST(ThreadPoolUsageExamples, ManyProducersStressAtomicCounter)
	{
		constexpr int kPosters = 12;
		constexpr int kTasksEach = 400;
		std::atomic<long long> sum{0};
		{
			FixedThreadPool pool(6);
			std::vector<std::thread> posters;
			posters.reserve(kPosters);
			for (int p = 0; p < kPosters; ++p)
			{
				posters.emplace_back([&pool, &sum] {
					for (int i = 0; i < kTasksEach; ++i)
					{
						pool.submit_void([&sum] { sum.fetch_add(1, std::memory_order_relaxed); });
					}
				});
			}
			for (auto& t : posters) t.join();
		}
		EXPECT_EQ(sum.load(), static_cast<long long>(kPosters * kTasksEach));
	}

	TEST(ThreadPoolUsageExamples, ParallelMapStyleFutures)
	{
		FixedThreadPool pool(4);
		std::vector<std::future<int>> futs;
		futs.reserve(8);
		for (int i = 0; i < 8; ++i)
		{
			futs.push_back(pool.submit([i] { return i * i; }));
		}
		int acc = 0;
		for (std::size_t i = 0; i < futs.size(); ++i)
		{
			ASSERT_EQ(futs[i].wait_for(std::chrono::seconds(1)), std::future_status::ready);
			acc += futs[i].get();
		}
		EXPECT_EQ(acc, 0 + 1 + 4 + 9 + 16 + 25 + 36 + 49);
	}

	TEST(ThreadPoolUsageExamples, ZeroWorkerCountThrows)
	{
		EXPECT_THROW(static_cast<void>(FixedThreadPool(0)), std::invalid_argument);
	}

} // namespace

/*
 * Further exercises
 * -----------------
 * 1. Add a **bounded** queue with `try_submit` / blocking `submit_or_wait`.
 * 2. Implement **work stealing** with per-thread deques and measure speedup on
 *    recursive divide-and-conquer workloads.
 * 3. Use **`std::jthread`** + `std::stop_token` for cooperative shutdown and
 *    interruptible waits.
 * 4. Demonstrate **pool self-deadlock** with a task that `future::get()`s another
 *    task on the same pool — then fix with a second pool or `async` depth limit.
 */

