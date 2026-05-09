#include <gtest/gtest.h>

#include <deque>
#include <queue>
#include <string>

namespace {

TEST(QueueHeader, QueueAdapterCoreOperations)
{
	// std::queue is a FIFO adapter: push/emplace append at the back, while front
	// and pop observe/remove the oldest element.
	std::queue<int> q;
	EXPECT_TRUE(q.empty());
	EXPECT_EQ(q.size(), 0u);

	q.push(1);
	q.push(2);
	q.emplace(3);
	EXPECT_EQ(q.front(), 1);
	EXPECT_EQ(q.back(), 3);
	EXPECT_EQ(q.size(), 3u);

	q.pop();
	EXPECT_EQ(q.front(), 2);
	q.pop();
	q.pop();
	EXPECT_TRUE(q.empty());
}

TEST(QueueHeader, ConstructionFromUnderlyingContainer)
{
	// queue can be constructed from an explicit underlying container instance.
	std::deque<std::string> data{ "a", "b", "c" };
	std::queue<std::string> q(data);
	EXPECT_EQ(q.front(), "a");
	EXPECT_EQ(q.back(), "c");
	EXPECT_EQ(q.size(), 3u);
}

TEST(QueueHeader, SwapAndComparisonOperatorsWhenAvailable)
{
	// Adapter swap exchanges underlying container state. C++20 also gives
	// comparisons for container adaptors where supported.
	std::queue<int> a;
	std::queue<int> b;
	a.push(1);
	a.push(2);
	b.push(9);

	a.swap(b);
	EXPECT_EQ(a.front(), 9);
	EXPECT_EQ(b.front(), 1);

	using std::swap;
	swap(a, b);
	EXPECT_EQ(a.front(), 1);
}

TEST(QueueHeader, PriorityQueueDefaultAndCustomComparatorBehavior)
{
	// std::priority_queue is a heap-based adapter. Default comparator creates a
	// max-heap, while std::greater builds a min-heap.
	std::priority_queue<int> maxq;
	maxq.push(3);
	maxq.push(1);
	maxq.push(7);
	EXPECT_EQ(maxq.top(), 7);
	maxq.pop();
	EXPECT_EQ(maxq.top(), 3);

	std::priority_queue<int, std::vector<int>, std::greater<int>> minq;
	minq.push(3);
	minq.push(1);
	minq.push(7);
	EXPECT_EQ(minq.top(), 1);
	minq.pop();
	EXPECT_EQ(minq.top(), 3);
}

TEST(QueueHeader, PriorityQueueFromRangeAndPushPopSemantics)
{
	// priority_queue can be built from a range and maintains heap ordering.
	std::vector<int> src{ 5, 2, 8, 1 };
	std::priority_queue<int> pq(src.begin(), src.end());
	EXPECT_EQ(pq.size(), 4u);
	EXPECT_EQ(pq.top(), 8);

	pq.emplace(9);
	EXPECT_EQ(pq.top(), 9);
	pq.pop();
	EXPECT_EQ(pq.top(), 8);
}

}  // namespace
