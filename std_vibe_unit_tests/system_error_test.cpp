#include <gtest/gtest.h>

#include <string>
#include <system_error>
#include <type_traits>

namespace {

	class SyntheticCategory final : public std::error_category
	{
	public:
		const char* name() const noexcept override
		{
			return "synthetic_category";
		}

		std::string message(int ev) const override
		{
			return "synthetic message " + std::to_string(ev);
		}

		std::error_condition default_error_condition(int ev) const noexcept override
		{
			return std::error_condition(ev, *this);
		}

		bool equivalent(int code, const std::error_condition& condition) const noexcept override
		{
			return condition.category() == *this && condition.value() == code;
		}

		bool equivalent(const std::error_code& code, int condition) const noexcept override
		{
			return code.category() == *this && code.value() == condition;
		}
	};

	const SyntheticCategory& GetSyntheticCategory()
	{
		static const SyntheticCategory category{};
		return category;
	}

	TEST(SystemError, ErrorCategoryIdentityNameMessageAndComparison)
	{
		// std::error_category is the polymorphic root for category-specific error
		// semantics. Categories compare by identity and provide name/message mapping.
		const std::error_category& generic = std::generic_category();
		const std::error_category& system = std::system_category();
		const std::error_category& synthetic = GetSyntheticCategory();

		EXPECT_NE(std::string(generic.name()).size(), 0u);
		EXPECT_NE(std::string(system.name()).size(), 0u);
		EXPECT_EQ(std::string(synthetic.name()), "synthetic_category");
		EXPECT_EQ(synthetic.message(7), "synthetic message 7");

		EXPECT_EQ(generic, std::generic_category());
		EXPECT_EQ(system, std::system_category());
		EXPECT_NE(generic, system);
		EXPECT_NE(synthetic, generic);
	}

	TEST(SystemError, ErrorCodeConstructionAssignmentObserversAndBooleanState)
	{
		// std::error_code stores an integer value + category and offers observer
		// accessors. Default construction is success (value==0, false in boolean
		// context); non-zero values become truthy.
		std::error_code ec;
		EXPECT_EQ(ec.value(), 0);
		EXPECT_EQ(ec.category(), std::system_category());
		EXPECT_FALSE(static_cast<bool>(ec));

		ec.assign(5, GetSyntheticCategory());
		EXPECT_EQ(ec.value(), 5);
		EXPECT_EQ(ec.category(), GetSyntheticCategory());
		EXPECT_TRUE(static_cast<bool>(ec));
		EXPECT_EQ(ec.message(), "synthetic message 5");

		ec.clear();
		EXPECT_EQ(ec.value(), 0);
		EXPECT_EQ(ec.category(), std::system_category());
		EXPECT_FALSE(static_cast<bool>(ec));
	}

	TEST(SystemError, ErrorConditionConstructionAssignmentObserversAndBooleanState)
	{
		// std::error_condition represents portable semantic conditions. Like
		// error_code, default state is success and non-zero values are truthy.
		std::error_condition cond;
		EXPECT_EQ(cond.value(), 0);
		EXPECT_EQ(cond.category(), std::generic_category());
		EXPECT_FALSE(static_cast<bool>(cond));

		cond.assign(9, GetSyntheticCategory());
		EXPECT_EQ(cond.value(), 9);
		EXPECT_EQ(cond.category(), GetSyntheticCategory());
		EXPECT_TRUE(static_cast<bool>(cond));
		EXPECT_EQ(cond.message(), "synthetic message 9");

		cond.clear();
		EXPECT_EQ(cond.value(), 0);
		EXPECT_EQ(cond.category(), std::generic_category());
		EXPECT_FALSE(static_cast<bool>(cond));
	}

	TEST(SystemError, ErrorCodeAndErrorConditionComparisonsAndEquivalence)
	{
		// Equality compares both value and category. Cross-type equivalence uses
		// category hooks (default_error_condition/equivalent) to relate code/condition.
		const std::error_code codeA(3, GetSyntheticCategory());
		const std::error_code codeB(3, GetSyntheticCategory());
		const std::error_code codeDifferentValue(4, GetSyntheticCategory());
		const std::error_condition conditionA(3, GetSyntheticCategory());

		EXPECT_EQ(codeA, codeB);
		EXPECT_NE(codeA, codeDifferentValue);
		EXPECT_EQ(conditionA, std::error_condition(3, GetSyntheticCategory()));
		EXPECT_TRUE(codeA == conditionA);
		EXPECT_TRUE(conditionA == codeA);
		EXPECT_FALSE(codeDifferentValue == conditionA);
	}

	TEST(SystemError, GenericHelpersForErrcAndTraitDetection)
	{
		// <system_error> provides errc plus generic helper factories and traits that
		// mark enums as convertible to error_code/error_condition.
		EXPECT_TRUE((std::is_error_condition_enum_v<std::errc>));
		EXPECT_FALSE((std::is_error_code_enum_v<std::errc>));

		const std::error_condition cond = std::make_error_condition(std::errc::invalid_argument);
		EXPECT_EQ(cond.category(), std::generic_category());
		EXPECT_EQ(cond, std::errc::invalid_argument);

		const std::error_code code = std::make_error_code(std::errc::permission_denied);
		EXPECT_EQ(code.category(), std::generic_category());
		EXPECT_EQ(code.default_error_condition(), std::errc::permission_denied);
	}

	TEST(SystemError, ErrorCodeAndConditionHashSupport)
	{
		// std::hash specializations exist for error_code and error_condition so they
		// can be used as keys in unordered containers.
		const std::error_code code(13, GetSyntheticCategory());
		const std::error_code codeSame(13, GetSyntheticCategory());
		const std::error_condition condition(13, GetSyntheticCategory());
		const std::error_condition conditionSame(13, GetSyntheticCategory());

		EXPECT_EQ(std::hash<std::error_code>{}(code), std::hash<std::error_code>{}(codeSame));
		EXPECT_EQ(std::hash<std::error_condition>{}(condition), std::hash<std::error_condition>{}(conditionSame));
	}

	TEST(SystemError, SystemErrorConstructorsWhatAndAccessors)
	{
		// std::system_error packages an error_code into an exception object with
		// optional explanatory text. Accessors must preserve stored code/value.
		const std::error_code code = std::make_error_code(std::errc::invalid_argument);

		const std::system_error fromCode(code);
		EXPECT_EQ(fromCode.code(), code);
		EXPECT_EQ(fromCode.code().value(), code.value());
		EXPECT_NE(std::string(fromCode.what()).size(), 0u);

		const std::system_error withMessage(code, "custom-prefix");
		EXPECT_EQ(withMessage.code(), code);
		EXPECT_NE(std::string(withMessage.what()).find("custom-prefix"), std::string::npos);

		const std::system_error fromValueAndCategory(EACCES, std::generic_category(), "access-error");
		EXPECT_EQ(fromValueAndCategory.code().value(), EACCES);
		EXPECT_EQ(fromValueAndCategory.code().category(), std::generic_category());
		EXPECT_NE(std::string(fromValueAndCategory.what()).find("access-error"), std::string::npos);
	}

	TEST(SystemError, ThrowAndCatchSystemErrorThroughRuntimeError)
	{
		// system_error derives from runtime_error, enabling broad catch strategies
		// while preserving typed code() access when caught specifically.
		EXPECT_TRUE((std::is_base_of_v<std::runtime_error, std::system_error>));

		try
		{
			throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory), "missing");
		}
		catch (const std::runtime_error& ex)
		{
			EXPECT_NE(std::string(ex.what()).size(), 0u);
		}
		catch (...)
		{
			FAIL() << "Expected std::system_error to be caught as std::runtime_error.";
		}
	}

}  // namespace
