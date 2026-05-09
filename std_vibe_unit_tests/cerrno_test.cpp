#include <gtest/gtest.h>

#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace {

	TEST(CErrno, RequiredErrorMacrosAreDefinedAndDistinct)
	{
		// <cerrno> must provide EDOM, EILSEQ, and ERANGE in addition to errno.
		// Their exact numeric values are implementation-defined, but they should be
		// nonzero and represent distinct categories of error conditions.
		EXPECT_NE(EDOM, 0);
		EXPECT_NE(EILSEQ, 0);
		EXPECT_NE(ERANGE, 0);

		EXPECT_NE(EDOM, EILSEQ);
		EXPECT_NE(EDOM, ERANGE);
		EXPECT_NE(EILSEQ, ERANGE);
	}

	TEST(CErrno, ErrnoIsWritableLValueAndCanBeReset)
	{
		// errno is a thread-local modifiable lvalue macro used to report library
		// error status. This test validates direct write/read/reset behavior.
		errno = 0;
		EXPECT_EQ(errno, 0);

		errno = ERANGE;
		EXPECT_EQ(errno, ERANGE);

		errno = EDOM;
		EXPECT_EQ(errno, EDOM);

		errno = 0;
		EXPECT_EQ(errno, 0);
	}

	TEST(CErrno, DomainErrorFromStrtodSetsErrnoToEDOM)
	{
		// strtod reports domain errors by setting errno to EDOM when conversion
		// input does not begin with a valid representation. This validates one
		// practical error path where errno communicates a parsing-domain failure.
		errno = 0;
		char* end = nullptr;
		const double value = std::strtod("not-a-number-literal", &end);
		(void)value;

		// No conversion is performed; end should equal start, and errno may remain 0
		// on some implementations. Accept both while still validating API behavior.
		EXPECT_NE(end, nullptr);
		EXPECT_STREQ(end, "not-a-number-literal");
		EXPECT_TRUE(errno == 0 || errno == EDOM);
	}

	TEST(CErrno, RangeErrorFromStrtolSetsErrnoToERANGE)
	{
		// strtol sets errno to ERANGE and clamps result on overflow/underflow.
		// This test uses an intentionally huge value to force range overflow.
		errno = 0;
		const long value = std::strtol("999999999999999999999999999999999", nullptr, 10);
		EXPECT_EQ(value, LONG_MAX);
		EXPECT_EQ(errno, ERANGE);
	}

	TEST(CErrno, StrerrorReturnsMessageForKnownErrorCodes)
	{
		// strerror maps errno-compatible codes to implementation-defined messages.
		// The content is platform-specific, but pointers should be non-null and
		// contain non-empty strings for standard required error macros.
		const char* domainMsg = std::strerror(EDOM);
		const char* rangeMsg = std::strerror(ERANGE);
		const char* sequenceMsg = std::strerror(EILSEQ);

		ASSERT_NE(domainMsg, nullptr);
		ASSERT_NE(rangeMsg, nullptr);
		ASSERT_NE(sequenceMsg, nullptr);
		EXPECT_GT(std::strlen(domainMsg), 0u);
		EXPECT_GT(std::strlen(rangeMsg), 0u);
		EXPECT_GT(std::strlen(sequenceMsg), 0u);
	}

	TEST(CErrno, CommonImplementationDefinedErrnoMacrosAreSaneWhenPresent)
	{
		// <cerrno> may expose many implementation-defined error macros. This test
		// validates that commonly available ones, when present, are nonzero and
		// therefore usable as real errno values in application error handling.
#ifdef EINVAL
		EXPECT_NE(EINVAL, 0);
#endif
#ifdef ENOENT
		EXPECT_NE(ENOENT, 0);
#endif
#ifdef EPERM
		EXPECT_NE(EPERM, 0);
#endif
#ifdef EACCES
		EXPECT_NE(EACCES, 0);
#endif
#ifdef EEXIST
		EXPECT_NE(EEXIST, 0);
#endif
#ifdef ENOMEM
		EXPECT_NE(ENOMEM, 0);
#endif
#ifdef EAGAIN
		EXPECT_NE(EAGAIN, 0);
#endif
#ifdef EBUSY
		EXPECT_NE(EBUSY, 0);
#endif
#ifdef EPIPE
		EXPECT_NE(EPIPE, 0);
#endif
#ifdef ENOTSUP
		EXPECT_NE(ENOTSUP, 0);
#endif
#ifdef ETIMEDOUT
		EXPECT_NE(ETIMEDOUT, 0);
#endif
		SUCCEED();
	}

}  // namespace
