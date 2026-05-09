#include <gtest/gtest.h>

#include <iosfwd>
#include <type_traits>

namespace {

TEST(IosfwdHeader, ForwardDeclaredStreamTypesAreVisible)
{
	// <iosfwd> exposes forward declarations and typedefs without heavy headers.
	std::istream* in = nullptr;
	std::ostream* out = nullptr;
	std::iostream* io = nullptr;
	std::ifstream* fin = nullptr;
	std::ofstream* fout = nullptr;
	std::stringstream* ss = nullptr;
	(void)in;
	(void)out;
	(void)io;
	(void)fin;
	(void)fout;
	(void)ss;
	SUCCEED();
}

TEST(IosfwdHeader, CharTraitsAndPosTypesAreDeclared)
{
	// Traits and stream position types are also forward-declared aliases.
	using traits = std::char_traits<char>;
	EXPECT_TRUE((std::is_class_v<traits>));

	using pos = std::streampos;
	using off = std::streamoff;
	EXPECT_TRUE((std::is_integral_v<off>));
	EXPECT_TRUE((std::is_default_constructible_v<pos>));
}

}  // namespace
