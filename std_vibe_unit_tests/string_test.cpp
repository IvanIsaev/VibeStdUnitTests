#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace {

TEST(StringHeader, ConstructionAssignmentAndConcatenation)
{
	// std::string supports rich construction and value-semantics assignment.
	std::string a;
	EXPECT_TRUE(a.empty());

	std::string b("hello");
	std::string c(3, 'x');
	std::string d = b + " " + c;
	EXPECT_EQ(d, "hello xxx");

	a = d;
	EXPECT_EQ(a, d);
}

TEST(StringHeader, ElementAccessCapacityAndModifiers)
{
	// Capacity growth and in-place modifiers are central to string usage.
	std::string s = "abc";
	EXPECT_EQ(s.front(), 'a');
	EXPECT_EQ(s.back(), 'c');
	EXPECT_EQ(s.at(1), 'b');
	EXPECT_THROW((void)s.at(3), std::out_of_range);

	s.push_back('d');
	s.append("ef");
	s.insert(0, "Z");
	s.erase(1, 1);  // erase 'a'
	EXPECT_EQ(s, "Zbcdef");

	s.reserve(64);
	EXPECT_GE(s.capacity(), 64u);
	s.shrink_to_fit();
	EXPECT_GE(s.capacity(), s.size());
}

TEST(StringHeader, SearchSubstrCompareAndReplaceOperations)
{
	// find/rfind/starts_with/ends_with and replace provide text manipulation.
	std::string s = "one two three two";
	EXPECT_EQ(s.find("two"), 4u);
	EXPECT_EQ(s.rfind("two"), 14u);
	EXPECT_TRUE(s.starts_with("one"));
	EXPECT_TRUE(s.ends_with("two"));

	auto sub = s.substr(4, 3);
	EXPECT_EQ(sub, "two");
	EXPECT_EQ(s.compare(0, 3, "one"), 0);

	s.replace(4, 3, "TWO");
	EXPECT_EQ(s, "one TWO three two");
}

TEST(StringHeader, StringViewInteroperabilityAndNonMemberErase)
{
	// string interoperates with string_view and supports C++20 erase helpers.
	std::string s = "alpha-beta-gamma";
	std::string_view sv = s;
	EXPECT_EQ(sv.substr(0, 5), "alpha");

#ifdef __cpp_lib_erase_if
	const auto removedDash = std::erase(s, '-');
	EXPECT_EQ(removedDash, 2u);
	const auto removedA = std::erase_if(s, [](char ch) { return ch == 'a'; });
	EXPECT_GT(removedA, 0u);
#endif
}

}  // namespace
