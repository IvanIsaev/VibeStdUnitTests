#include <gtest/gtest.h>

#include <deque>
#include <stack>
#include <string>

namespace {

TEST(StackHeader, CoreLifoOperations)
{
	// std::stack is a LIFO adapter with top() observing the newest element.
	std::stack<int> s;
	EXPECT_TRUE(s.empty());
	EXPECT_EQ(s.size(), 0u);

	s.push(1);
	s.push(2);
	s.emplace(3);
	EXPECT_EQ(s.top(), 3);
	EXPECT_EQ(s.size(), 3u);

	s.pop();
	EXPECT_EQ(s.top(), 2);
	s.pop();
	s.pop();
	EXPECT_TRUE(s.empty());
}

TEST(StackHeader, ConstructionFromUnderlyingContainer)
{
	// stack can be constructed from an existing underlying sequence container.
	std::deque<std::string> data{ "a", "b", "c" };
	std::stack<std::string> s(data);
	EXPECT_EQ(s.top(), "c");
	EXPECT_EQ(s.size(), 3u);
}

TEST(StackHeader, SwapAndComparisonBehavior)
{
	// stack supports member/non-member swap; relational operators compare
	// underlying containers lexicographically when available.
	std::stack<int> a;
	std::stack<int> b;
	a.push(1);
	a.push(2);
	b.push(9);

	a.swap(b);
	EXPECT_EQ(a.top(), 9);
	EXPECT_EQ(b.top(), 2);

	using std::swap;
	swap(a, b);
	EXPECT_EQ(a.top(), 2);
}

}  // namespace
