#include <gtest/gtest.h>

#include <memory>
#include <scoped_allocator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

	template <typename T>
	struct TagAllocator
	{
		using value_type = T;
		using propagate_on_container_copy_assignment = std::true_type;
		using propagate_on_container_move_assignment = std::true_type;
		using propagate_on_container_swap = std::true_type;
		using is_always_equal = std::false_type;

		int tag = 0;

		TagAllocator() = default;
		explicit TagAllocator(int t) : tag(t) {}

		template <typename U>
		TagAllocator(const TagAllocator<U>& other) : tag(other.tag) {}

		[[nodiscard]] T* allocate(std::size_t n)
		{
			return static_cast<T*>(::operator new(n * sizeof(T)));
		}

		void deallocate(T* p, std::size_t) noexcept
		{
			::operator delete(p);
		}

		template <typename U>
		bool operator==(const TagAllocator<U>& other) const noexcept
		{
			return tag == other.tag;
		}

		template <typename U>
		bool operator!=(const TagAllocator<U>& other) const noexcept
		{
			return !(*this == other);
		}
	};

	struct UsesInnerAllocator
	{
		using allocator_type = TagAllocator<int>;

		int payload = 0;
		int observedInnerTag = -1;

		UsesInnerAllocator(std::allocator_arg_t, const allocator_type& alloc, int value)
			: payload(value), observedInnerTag(alloc.tag) {}
	};

	TEST(ScopedAllocator, AdaptorTypeShapeAndNestedAllocatorAliases)
	{
		// <scoped_allocator> provides scoped_allocator_adaptor, which wraps an outer
		// allocator and optional inner allocators for nested allocator-aware objects.
		using Outer = TagAllocator<int>;
		using Inner = TagAllocator<UsesInnerAllocator>;
		using Scoped = std::scoped_allocator_adaptor<Outer, Inner>;

		EXPECT_TRUE((std::is_same_v<typename Scoped::outer_allocator_type, Outer>));
		EXPECT_TRUE((std::is_same_v<typename Scoped::inner_allocator_type, std::scoped_allocator_adaptor<Inner>>));
		EXPECT_TRUE((std::is_same_v<typename Scoped::value_type, int>));
	}

	TEST(ScopedAllocator, OuterAndInnerAllocatorObserversReflectConstructionArguments)
	{
		// outer_allocator() and inner_allocator() expose the allocator chain stored
		// by the adaptor, which is used to route allocations at each nesting level.
		using Outer = TagAllocator<int>;
		using Inner = TagAllocator<UsesInnerAllocator>;
		std::scoped_allocator_adaptor<Outer, Inner> scoped(Outer{ 11 }, Inner{ 22 });

		EXPECT_EQ(scoped.outer_allocator().tag, 11);
		EXPECT_EQ(scoped.inner_allocator().outer_allocator().tag, 22);
	}

	TEST(ScopedAllocator, RebindMaintainsAllocatorChainSemantics)
	{
		// scoped_allocator_adaptor supports rebinding through allocator_traits. The
		// rebound allocator should preserve the nested chain model for new value_type.
		using Outer = TagAllocator<int>;
		using Inner = TagAllocator<UsesInnerAllocator>;
		using Scoped = std::scoped_allocator_adaptor<Outer, Inner>;
		using Rebound = std::allocator_traits<Scoped>::template rebind_alloc<double>;

		EXPECT_TRUE((std::is_same_v<typename Rebound::outer_allocator_type, TagAllocator<double>>));
		EXPECT_TRUE((std::is_same_v<typename Rebound::inner_allocator_type, std::scoped_allocator_adaptor<Inner>>));
	}

	TEST(ScopedAllocator, TraitsPropagationFlagsMirrorAllocatorRequirements)
	{
		// Propagation and equality traits are surfaced through allocator_traits so
		// containers know how allocator state should move/copy/swap.
		using Scoped = std::scoped_allocator_adaptor<TagAllocator<int>>;
		using Traits = std::allocator_traits<Scoped>;

		EXPECT_TRUE((Traits::propagate_on_container_copy_assignment::value));
		EXPECT_TRUE((Traits::propagate_on_container_move_assignment::value));
		EXPECT_TRUE((Traits::propagate_on_container_swap::value));
		EXPECT_FALSE((Traits::is_always_equal::value));
	}

	TEST(ScopedAllocator, UsesAllocatorConstructionReceivesInnerAllocator)
	{
		// construct() on scoped_allocator_adaptor participates in uses-allocator
		// construction and should pass the *inner* allocator to nested objects.
		using Outer = TagAllocator<UsesInnerAllocator>;
		using Inner = TagAllocator<int>;
		std::scoped_allocator_adaptor<Outer, Inner> scoped(Outer{ 7 }, Inner{ 99 });

		Outer::value_type* ptr = scoped.allocate(1);
		ASSERT_NE(ptr, nullptr);
		scoped.construct(ptr, 123);
		EXPECT_EQ(ptr->payload, 123);
		EXPECT_EQ(ptr->observedInnerTag, 99);
		scoped.destroy(ptr);
		scoped.deallocate(ptr, 1);
	}

	TEST(ScopedAllocator, NestedContainerUsesScopedInnerAllocator)
	{
		// A primary use-case is nested containers: outer container allocator should
		// propagate an appropriate inner allocator to each contained allocator-aware
		// element (e.g., vector<string, ...>).
		using CharAlloc = TagAllocator<char>;
		using StringAlloc = std::scoped_allocator_adaptor<TagAllocator<std::basic_string<char, std::char_traits<char>, CharAlloc>>, CharAlloc>;
		using ScopedString = std::basic_string<char, std::char_traits<char>, CharAlloc>;
		using VectorAlloc = std::scoped_allocator_adaptor<TagAllocator<ScopedString>, CharAlloc>;

		VectorAlloc alloc(TagAllocator<ScopedString>{ 1 }, CharAlloc{ 77 });
		std::vector<ScopedString, VectorAlloc> values(alloc);
		values.emplace_back("hello");
		values.emplace_back("world");

		EXPECT_EQ(values.size(), 2u);
		EXPECT_EQ(values[0], "hello");
		EXPECT_EQ(values[1], "world");
		EXPECT_EQ(values[0].get_allocator().tag, 77);
		EXPECT_EQ(values[1].get_allocator().tag, 77);
	}

	TEST(ScopedAllocator, SelectOnContainerCopyConstructionIsWellFormed)
	{
		// scoped_allocator_adaptor exposes select_on_container_copy_construction,
		// which containers use to choose allocator state when copy-constructing.
		using Scoped = std::scoped_allocator_adaptor<TagAllocator<int>, TagAllocator<double>>;
		Scoped scoped(TagAllocator<int>{ 5 }, TagAllocator<double>{ 6 });

		Scoped selected = scoped.select_on_container_copy_construction();
		EXPECT_EQ(selected.outer_allocator().tag, 5);
		EXPECT_EQ(selected.inner_allocator().outer_allocator().tag, 6);
	}

}  // namespace
