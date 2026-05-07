#include <gtest/gtest.h>

#include <coroutine>
#include <exception>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace usage_examples::coroutines {

	// Example 1: "fire-and-forget" coroutine return type.
	// Use this pattern when you want a coroutine that starts immediately and does
	// not expose a value to the caller. It is useful for side-effect work.
	struct FireAndForget
	{
		struct promise_type
		{
			FireAndForget get_return_object() noexcept { return {}; }
			std::suspend_never initial_suspend() noexcept { return {}; }
			std::suspend_never final_suspend() noexcept { return {}; }
			void return_void() noexcept {}
			void unhandled_exception() noexcept {}
		};
	};

	FireAndForget ImmediateSideEffectCounter(int& counter)
	{
		// Because initial_suspend returns suspend_never, the coroutine body runs
		// immediately when called.
		++counter;
		co_return;
	}

	// Example 2: Lazy single-value task with explicit resume().
	// This pattern is a small building block for async pipelines where execution
	// should start only when the caller decides.
	template <typename T>
	struct LazyTask
	{
		struct promise_type
		{
			T value{};
			std::exception_ptr error;

			LazyTask get_return_object() noexcept
			{
				return LazyTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
			}

			std::suspend_always initial_suspend() noexcept { return {}; }
			std::suspend_always final_suspend() noexcept { return {}; }
			void return_value(T v) noexcept { value = std::move(v); }
			void unhandled_exception() noexcept { error = std::current_exception(); }
		};

		std::coroutine_handle<promise_type> handle{};

		explicit LazyTask(std::coroutine_handle<promise_type> h) : handle(h) {}
		LazyTask(const LazyTask&) = delete;
		LazyTask& operator=(const LazyTask&) = delete;
		LazyTask(LazyTask&& other) noexcept : handle(other.handle) { other.handle = {}; }
		LazyTask& operator=(LazyTask&& other) noexcept
		{
			if (this != &other)
			{
				if (handle) handle.destroy();
				handle = other.handle;
				other.handle = {};
			}
			return *this;
		}
		~LazyTask()
		{
			if (handle) handle.destroy();
		}

		bool done() const { return !handle || handle.done(); }

		void resume()
		{
			if (handle && !handle.done())
			{
				handle.resume();
			}
		}

		T result()
		{
			if (handle.promise().error)
			{
				std::rethrow_exception(handle.promise().error);
			}
			return handle.promise().value;
		}
	};

	LazyTask<int> ComputeValueLazily(int base)
	{
		// Body runs only after caller invokes task.resume().
		co_return base * 2;
	}

	// Example 3: Minimal pull-based generator.
	// Generator is ideal when you want to produce values incrementally without
	// materializing the entire sequence in memory.
	template <typename T>
	struct Generator
	{
		struct promise_type
		{
			std::optional<T> current;

			Generator get_return_object() noexcept
			{
				return Generator{ std::coroutine_handle<promise_type>::from_promise(*this) };
			}
			std::suspend_always initial_suspend() noexcept { return {}; }
			std::suspend_always final_suspend() noexcept { return {}; }
			std::suspend_always yield_value(T v) noexcept
			{
				current = std::move(v);
				return {};
			}
			void return_void() noexcept {}
			void unhandled_exception() noexcept { std::terminate(); }
		};

		std::coroutine_handle<promise_type> handle{};

		explicit Generator(std::coroutine_handle<promise_type> h) : handle(h) {}
		Generator(const Generator&) = delete;
		Generator& operator=(const Generator&) = delete;
		Generator(Generator&& other) noexcept : handle(other.handle) { other.handle = {}; }
		~Generator()
		{
			if (handle) handle.destroy();
		}

		bool next()
		{
			if (!handle || handle.done()) return false;
			handle.resume();
			return !handle.done();
		}

		const T& value() const { return *handle.promise().current; }
	};

	Generator<int> Range(int start, int endExclusive)
	{
		for (int i = start; i < endExclusive; ++i)
		{
			co_yield i;
		}
	}

	// Example 4: Custom awaiter that inserts a manual "delay point".
	// In real async runtimes this would register a timer with an event loop.
	// Here it simply suspends once and requires caller-driven resume().
	struct OneShotSuspend
	{
		bool await_ready() const noexcept { return false; }
		void await_suspend(std::coroutine_handle<>) const noexcept {}
		void await_resume() const noexcept {}
	};

	LazyTask<int> TwoPhaseComputation()
	{
		// Execution pauses here at first resume, then continues on second resume.
		co_await OneShotSuspend{};
		co_return 99;
	}

	// Example 5: Make a task awaitable by another coroutine.
	// This lets you "chain" async computations with co_await.
	template <typename T>
	struct AwaitableTask
	{
		struct promise_type
		{
			T value{};
			std::exception_ptr error;

			AwaitableTask get_return_object() noexcept
			{
				return AwaitableTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
			}
			std::suspend_never initial_suspend() noexcept { return {}; }
			std::suspend_never final_suspend() noexcept { return {}; }
			void return_value(T v) noexcept { value = std::move(v); }
			void unhandled_exception() noexcept { error = std::current_exception(); }
		};

		std::coroutine_handle<promise_type> handle{};

		explicit AwaitableTask(std::coroutine_handle<promise_type> h) : handle(h) {}
		AwaitableTask(const AwaitableTask&) = delete;
		AwaitableTask& operator=(const AwaitableTask&) = delete;
		AwaitableTask(AwaitableTask&& other) noexcept : handle(other.handle) { other.handle = {}; }
		~AwaitableTask()
		{
			if (handle) handle.destroy();
		}

		bool await_ready() const noexcept { return true; }
		void await_suspend(std::coroutine_handle<>) const noexcept {}
		T await_resume()
		{
			if (handle.promise().error) std::rethrow_exception(handle.promise().error);
			return handle.promise().value;
		}

		T result()
		{
			if (handle.promise().error) std::rethrow_exception(handle.promise().error);
			return handle.promise().value;
		}
	};

	AwaitableTask<int> AddFive(int x)
	{
		co_return x + 5;
	}

	AwaitableTask<int> ChainWithAwait(int x)
	{
		// Demonstrates natural control flow when composing awaitable tasks.
		const int y = co_await AddFive(x);
		co_return y * 3;
	}

	// Example 6: Exception propagation from coroutine to caller.
	LazyTask<int> ThrowingTask()
	{
		throw std::runtime_error("coroutine failure");
		co_return 0;
	}

	// Example 7: Cooperative cancellation with a shared flag.
	// Coroutines are best cancelled cooperatively: the body periodically checks a
	// token/flag and exits early.
	struct CancellationFlag
	{
		bool cancelled = false;
	};

	Generator<int> CountUntilCancelled(CancellationFlag& flag, int maxValue)
	{
		for (int i = 0; i < maxValue; ++i)
		{
			if (flag.cancelled) co_return;
			co_yield i;
		}
	}

	// Example 8: Use noop_coroutine as a safe placeholder continuation.
	// noop_coroutine never completes and can be resumed/destroyed safely, making
	// it useful as a default handle in framework internals.
	bool IsNoopHandleStable()
	{
		auto h = std::noop_coroutine();
		void* before = h.address();
		h.resume();
		h.destroy();
		return before == h.address();
	}

	// Small aggregation function that "uses" examples in one place.
	// This is convenient for readers who want a compact walkthrough entry point.
	std::vector<int> RunCoroutineExamples()
	{
		std::vector<int> results;

		int sideEffects = 0;
		ImmediateSideEffectCounter(sideEffects);
		results.push_back(sideEffects);  // expects 1

		auto lazy = ComputeValueLazily(10);
		lazy.resume();
		results.push_back(lazy.result());  // expects 20

		auto twoPhase = TwoPhaseComputation();
		twoPhase.resume();  // reaches suspension point
		twoPhase.resume();  // completes and produces value
		results.push_back(twoPhase.result());  // expects 99

		auto chained = ChainWithAwait(10);
		results.push_back(chained.result());  // expects (10 + 5) * 3 = 45

		auto seq = Range(3, 6);
		while (seq.next()) results.push_back(seq.value());  // 3, 4, 5

		CancellationFlag cancel{};
		auto cancellable = CountUntilCancelled(cancel, 10);
		if (cancellable.next()) results.push_back(cancellable.value());  // 0
		if (cancellable.next()) results.push_back(cancellable.value());  // 1
		cancel.cancelled = true;
		cancellable.next();  // stops producing values

		results.push_back(IsNoopHandleStable() ? 1 : 0);
		return results;
	}

}  // namespace usage_examples::coroutines

namespace {

	TEST(CoroutineUsageExamples, AggregatedFlowProducesExpectedSequence)
	{
		// This end-to-end test executes the compact walkthrough helper and verifies
		// the exact sequence of produced values. It effectively validates that the
		// fire-and-forget behavior, lazy resumption, chaining, generator yields,
		// cooperative cancellation, and noop handle stability all work as documented.
		const std::vector<int> values = usage_examples::coroutines::RunCoroutineExamples();
		const std::vector<int> expected{ 1, 20, 99, 45, 3, 4, 5, 0, 1, 1 };
		EXPECT_EQ(values, expected);
	}

	TEST(CoroutineUsageExamples, ThrowingTaskPropagatesStoredExceptionOnResult)
	{
		// LazyTask captures unhandled exceptions inside promise_type and rethrows
		// when result() is requested. This verifies the expected consumption model:
		// resume the coroutine to completion, then observe the propagated exception.
		auto task = usage_examples::coroutines::ThrowingTask();
		task.resume();
		EXPECT_THROW(task.result(), std::runtime_error);
	}

	TEST(CoroutineUsageExamples, GeneratorStopsAfterCancellation)
	{
		// Cooperative cancellation is demonstrated by flipping a shared flag.
		// The generator should emit values while flag is false and stop immediately
		// once cancellation is requested.
		usage_examples::coroutines::CancellationFlag cancel{};
		auto gen = usage_examples::coroutines::CountUntilCancelled(cancel, 5);

		ASSERT_TRUE(gen.next());
		EXPECT_EQ(gen.value(), 0);
		ASSERT_TRUE(gen.next());
		EXPECT_EQ(gen.value(), 1);

		cancel.cancelled = true;
		EXPECT_FALSE(gen.next());
	}

	TEST(CoroutineUsageExamples, TwoPhaseComputationRequiresTwoResumes)
	{
		// The custom awaiter in TwoPhaseComputation suspends once before producing
		// a value, so one resume reaches suspension and a second resume completes.
		auto task = usage_examples::coroutines::TwoPhaseComputation();
		EXPECT_FALSE(task.done());
		task.resume();
		EXPECT_FALSE(task.done());
		task.resume();
		EXPECT_TRUE(task.done());
		EXPECT_EQ(task.result(), 99);
	}

}  // namespace
