#include <gtest/gtest.h>

#include <coroutine>
#include <type_traits>

namespace {

	struct ManualTask
	{
		struct promise_type
		{
			int value = 0;

			ManualTask get_return_object()
			{
				return ManualTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
			}

			std::suspend_always initial_suspend() noexcept { return {}; }
			std::suspend_always final_suspend() noexcept { return {}; }
			void return_value(int v) noexcept { value = v; }
			void unhandled_exception() noexcept {}
		};

		std::coroutine_handle<promise_type> h{};

		explicit ManualTask(std::coroutine_handle<promise_type> handle) : h(handle) {}
		ManualTask(ManualTask&& other) noexcept : h(other.h) { other.h = {}; }
		ManualTask& operator=(ManualTask&& other) noexcept
		{
			if (this != &other)
			{
				if (h) h.destroy();
				h = other.h;
				other.h = {};
			}
			return *this;
		}

		ManualTask(const ManualTask&) = delete;
		ManualTask& operator=(const ManualTask&) = delete;

		~ManualTask()
		{
			if (h) h.destroy();
		}
	};

	ManualTask MakeManualTask(int v)
	{
		co_return v;
	}

	TEST(Coroutine, AwaiterUtilityTypes)
	{
		// <coroutine> defines awaiter utility types `suspend_never` and
		// `suspend_always`. This test verifies their awaiter protocol behavior:
		// `suspend_never` does not suspend and reports ready immediately, while
		// `suspend_always` requests suspension and reports not-ready.
		std::suspend_never never;
		std::suspend_always always;

		EXPECT_TRUE(never.await_ready());
		EXPECT_FALSE(always.await_ready());

		never.await_resume();
		always.await_resume();
	}

	TEST(Coroutine, CoroutineTraitsResolvesPromiseType)
	{
		// `std::coroutine_traits<R, Args...>` maps a coroutine return type to the
		// required `promise_type`. This test ensures our coroutine return object
		// (`ManualTask`) is correctly associated with `ManualTask::promise_type`,
		// validating the type-level contract used by coroutine transformation.
		using Traits = std::coroutine_traits<ManualTask, int>;
		EXPECT_TRUE((std::is_same_v<typename Traits::promise_type, ManualTask::promise_type>));
	}

	TEST(Coroutine, HandleFromPromiseAddressAndFromAddress)
	{
		// `std::coroutine_handle<P>` provides conversion hooks between promise and
		// handle (`from_promise`), and raw frame address (`address`/`from_address`).
		// This test resumes a suspended coroutine once, checks produced value through
		// promise access, and verifies round-trip frame address conversion.
		ManualTask task = MakeManualTask(42);
		ASSERT_TRUE(task.h);
		EXPECT_FALSE(task.h.done());

		task.h.resume();
		EXPECT_TRUE(task.h.done());
		EXPECT_EQ(task.h.promise().value, 42);

		void* frame = task.h.address();
		ASSERT_NE(frame, nullptr);
		auto same = std::coroutine_handle<ManualTask::promise_type>::from_address(frame);
		EXPECT_EQ(same.address(), frame);
	}

	TEST(Coroutine, GenericHandleConversionsAndComparison)
	{
		// Typed handles are implicitly convertible to untyped
		// `std::coroutine_handle<>`. This test verifies conversion and equality
		// semantics by comparing underlying frame addresses and confirms null/default
		// handles represent an empty frame.
		ManualTask task = MakeManualTask(7);
		ASSERT_TRUE(task.h);

		std::coroutine_handle<> generic = task.h;
		EXPECT_EQ(generic.address(), task.h.address());

		std::coroutine_handle<> empty;
		EXPECT_FALSE(empty);
		EXPECT_NE(empty.address(), generic.address());
	}

	TEST(Coroutine, DestroyClearsOwnershipByConvention)
	{
		// `destroy()` releases coroutine frame storage for a valid handle. In this
		// project wrapper we manually clear the stored handle after destroy so that
		// wrapper destruction remains safe and idempotent. This test validates that
		// explicit destruction path works and ownership is reset.
		ManualTask task = MakeManualTask(5);
		ASSERT_TRUE(task.h);

		task.h.destroy();
		task.h = {};
		EXPECT_FALSE(task.h);
	}

	TEST(Coroutine, NoopCoroutineHandleBehavior)
	{
		// `std::noop_coroutine()` yields a special no-op coroutine handle whose
		// operations are safe and side-effect free. This test checks stable properties:
		// handle validity, done-state, self-consistent promise access, and resume/
		// destroy availability without changing its fundamental identity.
		auto h = std::noop_coroutine();
		ASSERT_TRUE(h);
		EXPECT_FALSE(h.done());
		EXPECT_NE(h.address(), nullptr);

		auto before = h.address();
		h.resume();
		h.destroy();
		EXPECT_EQ(h.address(), before);
	}

	TEST(Coroutine, NoopCoroutineHandleTypeAliases)
	{
		// <coroutine> defines `noop_coroutine_handle` as a convenience alias for
		// `std::coroutine_handle<std::noop_coroutine_promise>`. This test verifies the
		// alias relationship and ensures `std::noop_coroutine()` returns that type.
		EXPECT_TRUE((std::is_same_v<
			std::noop_coroutine_handle,
			std::coroutine_handle<std::noop_coroutine_promise>>));

		EXPECT_TRUE((std::is_same_v<
			decltype(std::noop_coroutine()),
			std::noop_coroutine_handle>));
	}

}  // namespace
