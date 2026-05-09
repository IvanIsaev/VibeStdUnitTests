#include <gtest/gtest.h>

#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace {

	TEST(StdExcept, LogicErrorHierarchyContracts)
	{
		// <stdexcept> defines logic_error and its argument/domain/length/range
		// derivatives for errors that are theoretically detectable before runtime.
		// This test validates inheritance relationships used by catch hierarchies.
		EXPECT_TRUE((std::is_base_of_v<std::exception, std::logic_error>));
		EXPECT_TRUE((std::is_base_of_v<std::logic_error, std::invalid_argument>));
		EXPECT_TRUE((std::is_base_of_v<std::logic_error, std::domain_error>));
		EXPECT_TRUE((std::is_base_of_v<std::logic_error, std::length_error>));
		EXPECT_TRUE((std::is_base_of_v<std::logic_error, std::out_of_range>));
	}

	TEST(StdExcept, RuntimeErrorHierarchyContracts)
	{
		// runtime_error and its range/overflow/underflow derivatives model failures
		// generally detectable only during execution. This verifies the hierarchy.
		EXPECT_TRUE((std::is_base_of_v<std::exception, std::runtime_error>));
		EXPECT_TRUE((std::is_base_of_v<std::runtime_error, std::range_error>));
		EXPECT_TRUE((std::is_base_of_v<std::runtime_error, std::overflow_error>));
		EXPECT_TRUE((std::is_base_of_v<std::runtime_error, std::underflow_error>));
	}

	TEST(StdExcept, LogicErrorPreservesMessageFromStringAndCStringConstructors)
	{
		// Each <stdexcept> exception type stores a human-readable explanatory
		// message returned by what(). This test checks both std::string and C-string
		// constructor forms on the root logic_error type.
		const std::logic_error fromString(std::string("logic-string-message"));
		const std::logic_error fromCString("logic-cstring-message");

		EXPECT_STREQ(fromString.what(), "logic-string-message");
		EXPECT_STREQ(fromCString.what(), "logic-cstring-message");
	}

	TEST(StdExcept, InvalidArgumentConstructAndThrowBehavior)
	{
		// invalid_argument reports invalid input parameters. We validate direct what()
		// payload preservation and typed throw/catch behavior through base reference.
		const std::invalid_argument direct("invalid-argument-message");
		EXPECT_STREQ(direct.what(), "invalid-argument-message");

		try
		{
			throw std::invalid_argument("bad-arg");
		}
		catch (const std::logic_error& ex)
		{
			EXPECT_STREQ(ex.what(), "bad-arg");
		}
		catch (...)
		{
			FAIL() << "Expected std::invalid_argument to be caught as std::logic_error.";
		}
	}

	TEST(StdExcept, DomainErrorConstructAndThrowBehavior)
	{
		// domain_error indicates mathematically invalid domains (e.g., sqrt(-1) in
		// real-valued contexts). This test checks message propagation and hierarchy.
		const std::domain_error direct("domain-message");
		EXPECT_STREQ(direct.what(), "domain-message");

		EXPECT_THROW(throw std::domain_error("domain-throw"), std::domain_error);
	}

	TEST(StdExcept, LengthErrorConstructAndThrowBehavior)
	{
		// length_error signals that an object would exceed implementation or logical
		// size limits (for example, oversized container/string requests).
		const std::length_error direct("length-message");
		EXPECT_STREQ(direct.what(), "length-message");

		EXPECT_THROW(throw std::length_error("length-throw"), std::length_error);
	}

	TEST(StdExcept, OutOfRangeConstructAndThrowBehavior)
	{
		// out_of_range reports bounds violations such as invalid index access. We
		// verify payload persistence and that it can be handled as logic_error.
		const std::out_of_range direct("out-of-range-message");
		EXPECT_STREQ(direct.what(), "out-of-range-message");

		try
		{
			throw std::out_of_range("index-error");
		}
		catch (const std::logic_error& ex)
		{
			EXPECT_STREQ(ex.what(), "index-error");
		}
		catch (...)
		{
			FAIL() << "Expected std::out_of_range to be caught as std::logic_error.";
		}
	}

	TEST(StdExcept, RuntimeErrorPreservesMessageFromConstructors)
	{
		// runtime_error is the root for execution-time failures in <stdexcept>.
		// Validate both constructor forms and what() payload forwarding.
		const std::runtime_error fromString(std::string("runtime-string-message"));
		const std::runtime_error fromCString("runtime-cstring-message");

		EXPECT_STREQ(fromString.what(), "runtime-string-message");
		EXPECT_STREQ(fromCString.what(), "runtime-cstring-message");
	}

	TEST(StdExcept, RangeErrorConstructAndThrowBehavior)
	{
		// range_error indicates representable-range problems discovered at runtime.
		const std::range_error direct("range-message");
		EXPECT_STREQ(direct.what(), "range-message");

		EXPECT_THROW(throw std::range_error("range-throw"), std::range_error);
	}

	TEST(StdExcept, OverflowErrorConstructAndThrowBehavior)
	{
		// overflow_error indicates arithmetic overflow in contexts that diagnose it.
		const std::overflow_error direct("overflow-message");
		EXPECT_STREQ(direct.what(), "overflow-message");

		EXPECT_THROW(throw std::overflow_error("overflow-throw"), std::overflow_error);
	}

	TEST(StdExcept, UnderflowErrorConstructAndThrowBehavior)
	{
		// underflow_error indicates arithmetic underflow in diagnosing contexts.
		const std::underflow_error direct("underflow-message");
		EXPECT_STREQ(direct.what(), "underflow-message");

		EXPECT_THROW(throw std::underflow_error("underflow-throw"), std::underflow_error);
	}

	TEST(StdExcept, CopyAndAssignmentPreserveWhatPayloadAcrossTypes)
	{
		// Standard exception objects from <stdexcept> are copyable. Copying and
		// assignment should preserve message text observable via what().
		const std::runtime_error original("copy-payload");
		const std::runtime_error copied(original);
		EXPECT_STREQ(copied.what(), "copy-payload");

		std::runtime_error assigned("before");
		assigned = original;
		EXPECT_STREQ(assigned.what(), "copy-payload");
	}

}  // namespace
