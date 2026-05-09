#include <gtest/gtest.h>

#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace {

	class SimpleException final : public std::exception
	{
	public:
		const char* what() const noexcept override
		{
			return "simple-exception";
		}
	};

	TEST(ExceptionHeader, ExceptionBaseClassSupportsWhatAndPolymorphism)
	{
		// std::exception is the root of the standard exception hierarchy. It is
		// polymorphic and provides a noexcept what() message interface consumed by
		// generic catch handlers. This test validates dynamic dispatch behavior and
		// message stability through a base-class reference.
		SimpleException ex;
		const std::exception& baseRef = ex;

		EXPECT_STREQ(baseRef.what(), "simple-exception");
		EXPECT_TRUE((std::is_polymorphic_v<std::exception>));
	}

	TEST(ExceptionHeader, BadExceptionIsCatchableAsStandardException)
	{
		// std::bad_exception participates in exception specifications and remains a
		// distinct type in the hierarchy. This test checks inheritance and that it
		// can be observed through the common std::exception interface.
		std::bad_exception bad;
		const std::exception& baseRef = bad;

		EXPECT_TRUE((std::is_base_of_v<std::exception, std::bad_exception>));
		EXPECT_NE(std::string(baseRef.what()).size(), 0u);
	}

	TEST(ExceptionHeader, TerminateHandlerRoundTripWithSetAndGet)
	{
		// terminate_handler is a function pointer type used by set_terminate and
		// get_terminate. We avoid calling std::terminate in tests (it would end the
		// process) and instead verify handler installation and restoration semantics.
		auto original = std::get_terminate();
		auto replacement = +[]() noexcept { std::abort(); };

		auto previous = std::set_terminate(replacement);
		EXPECT_EQ(previous, original);
		EXPECT_EQ(std::get_terminate(), replacement);

		auto restorePrevious = std::set_terminate(original);
		EXPECT_EQ(restorePrevious, replacement);
		EXPECT_EQ(std::get_terminate(), original);
	}

	TEST(ExceptionHeader, UncaughtExceptionsCounterOutsideThrowIsZero)
	{
		// std::uncaught_exceptions reports how many exception objects are currently
		// active on the calling thread. Outside stack unwinding and active throws,
		// this must be zero, which provides a safe baseline for scope-guard logic.
		EXPECT_EQ(std::uncaught_exceptions(), 0);
	}

	TEST(ExceptionHeader, ExceptionPtrCapturesAndRethrowsOriginalException)
	{
		// std::exception_ptr stores a copy/reference to an active exception and can
		// later rethrow it through std::rethrow_exception. This enables cross-layer
		// or deferred error propagation while preserving the dynamic exception type.
		std::exception_ptr captured;
		try
		{
			throw std::runtime_error("captured-runtime-error");
		}
		catch (...)
		{
			captured = std::current_exception();
		}

		ASSERT_NE(captured, nullptr);
		EXPECT_THROW(std::rethrow_exception(captured), std::runtime_error);

		try
		{
			std::rethrow_exception(captured);
		}
		catch (const std::runtime_error& ex)
		{
			EXPECT_STREQ(ex.what(), "captured-runtime-error");
		}
		catch (...)
		{
			FAIL() << "Expected std::runtime_error after rethrow_exception.";
		}
	}

	TEST(ExceptionHeader, MakeExceptionPtrCreatesTypedStoredException)
	{
		// std::make_exception_ptr creates an exception_ptr from a concrete exception
		// object without requiring an immediate throw/catch sequence. This test
		// verifies type preservation and payload integrity after rethrow.
		const std::exception_ptr ptr = std::make_exception_ptr(std::logic_error("logic-payload"));
		ASSERT_NE(ptr, nullptr);

		try
		{
			std::rethrow_exception(ptr);
		}
		catch (const std::logic_error& ex)
		{
			EXPECT_STREQ(ex.what(), "logic-payload");
		}
		catch (...)
		{
			FAIL() << "Expected std::logic_error from make_exception_ptr payload.";
		}
	}

	class NestedCarrier : public std::runtime_error, public std::nested_exception
	{
	public:
		explicit NestedCarrier(const std::string& message)
			: std::runtime_error(message), std::nested_exception() {}
	};

	TEST(ExceptionHeader, NestedExceptionCapturesAndRethrowsInnerException)
	{
		// std::nested_exception stores the currently handled exception, enabling a
		// causal chain. rethrow_nested re-emits the inner exception when present.
		// This test constructs an object while handling an inner exception and checks
		// that the nested payload can be recovered with the original message.
		NestedCarrier carrier("outer-message");
		try
		{
			throw std::runtime_error("inner-message");
		}
		catch (...)
		{
			carrier = NestedCarrier("outer-message");
		}

		try
		{
			carrier.rethrow_nested();
			FAIL() << "Expected rethrow_nested to throw captured inner exception.";
		}
		catch (const std::runtime_error& inner)
		{
			EXPECT_STREQ(inner.what(), "inner-message");
		}
		catch (...)
		{
			FAIL() << "Expected nested std::runtime_error.";
		}
	}

	TEST(ExceptionHeader, ThrowWithNestedAndRethrowIfNestedPreserveErrorChain)
	{
		// throw_with_nested wraps a new exception object while preserving the active
		// one as nested context. rethrow_if_nested conditionally unwraps that inner
		// exception. Together they provide standardized multi-layer error chaining.
		try
		{
			try
			{
				throw std::logic_error("inner-cause");
			}
			catch (...)
			{
				std::throw_with_nested(std::runtime_error("outer-layer"));
			}
		}
		catch (const std::runtime_error& outer)
		{
			EXPECT_STREQ(outer.what(), "outer-layer");

			try
			{
				std::rethrow_if_nested(outer);
				FAIL() << "Expected nested exception from std::rethrow_if_nested.";
			}
			catch (const std::logic_error& inner)
			{
				EXPECT_STREQ(inner.what(), "inner-cause");
			}
			catch (...)
			{
				FAIL() << "Expected nested std::logic_error.";
			}
		}
		catch (...)
		{
			FAIL() << "Expected outer std::runtime_error from throw_with_nested.";
		}
	}

}  // namespace
