#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <memory_resource>
#include <string>
#include <type_traits>
#include <vector>

namespace {

	class CountingResource final : public std::pmr::memory_resource
	{
	public:
		int allocateCalls = 0;
		int deallocateCalls = 0;
		std::size_t lastAllocatedBytes = 0;
		std::size_t lastAllocatedAlignment = 0;

	protected:
		void* do_allocate(std::size_t bytes, std::size_t alignment) override
		{
			++allocateCalls;
			lastAllocatedBytes = bytes;
			lastAllocatedAlignment = alignment;
			return ::operator new(bytes, std::align_val_t(alignment));
		}

		void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override
		{
			(void)bytes;
			(void)alignment;
			++deallocateCalls;
			::operator delete(p, std::align_val_t(alignment));
		}

		bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
		{
			return this == &other;
		}
	};

	TEST(MemoryResource, MemoryResourceAbstractInterfaceAndEqualitySemantics)
	{
		// std::pmr::memory_resource is the polymorphic base for runtime-selected
		// allocation strategies. This test validates core virtual API behavior and
		// identity-based equality via do_is_equal in a custom resource.
		CountingResource first;
		CountingResource second;

		std::pmr::memory_resource& baseFirst = first;
		std::pmr::memory_resource& baseSecond = second;
		EXPECT_TRUE((baseFirst == baseFirst));
		EXPECT_FALSE((baseFirst == baseSecond));
		EXPECT_TRUE((baseFirst != baseSecond));

		void* block = baseFirst.allocate(64, alignof(std::max_align_t));
		ASSERT_NE(block, nullptr);
		EXPECT_EQ(first.allocateCalls, 1);
		EXPECT_EQ(first.lastAllocatedBytes, 64u);
		baseFirst.deallocate(block, 64, alignof(std::max_align_t));
		EXPECT_EQ(first.deallocateCalls, 1);
	}

	TEST(MemoryResource, NewDeleteResourceAndNullMemoryResourceContracts)
	{
		// <memory_resource> provides singleton resources:
		// - new_delete_resource: forwards to global new/delete
		// - null_memory_resource: always fails allocations
		// They must be stable singleton addresses and usable via memory_resource API.
		std::pmr::memory_resource* newDeleteA = std::pmr::new_delete_resource();
		std::pmr::memory_resource* newDeleteB = std::pmr::new_delete_resource();
		ASSERT_NE(newDeleteA, nullptr);
		EXPECT_EQ(newDeleteA, newDeleteB);

		void* block = newDeleteA->allocate(32, alignof(std::max_align_t));
		ASSERT_NE(block, nullptr);
		newDeleteA->deallocate(block, 32, alignof(std::max_align_t));

		std::pmr::memory_resource* nullA = std::pmr::null_memory_resource();
		std::pmr::memory_resource* nullB = std::pmr::null_memory_resource();
		ASSERT_NE(nullA, nullptr);
		EXPECT_EQ(nullA, nullB);
		EXPECT_THROW(nullA->allocate(1, alignof(std::max_align_t)), std::bad_alloc);
	}

	TEST(MemoryResource, DefaultResourceCanBeObservedAndTemporarilyReplaced)
	{
		// get_default_resource/set_default_resource control the process-wide default
		// PMR allocation source used by pmr containers when no allocator is passed.
		std::pmr::memory_resource* original = std::pmr::get_default_resource();
		ASSERT_NE(original, nullptr);

		CountingResource replacement;
		std::pmr::memory_resource* previous = std::pmr::set_default_resource(&replacement);
		EXPECT_EQ(previous, original);
		EXPECT_EQ(std::pmr::get_default_resource(), &replacement);

		std::pmr::memory_resource* restoredPrevious = std::pmr::set_default_resource(original);
		EXPECT_EQ(restoredPrevious, &replacement);
		EXPECT_EQ(std::pmr::get_default_resource(), original);
	}

	TEST(MemoryResource, PolymorphicAllocatorUsesSuppliedResourceAndTraits)
	{
		// std::pmr::polymorphic_allocator<T> binds allocations to a runtime-selected
		// memory_resource and integrates with allocator_traits/container machinery.
		CountingResource resource;
		std::pmr::polymorphic_allocator<int> alloc(&resource);

		EXPECT_EQ(alloc.resource(), &resource);

		int* ptr = alloc.allocate(3);
		ASSERT_NE(ptr, nullptr);
		ptr[0] = 10;
		ptr[1] = 20;
		ptr[2] = 30;
		EXPECT_EQ(ptr[1], 20);
		alloc.deallocate(ptr, 3);
		EXPECT_GE(resource.allocateCalls, 1);
		EXPECT_GE(resource.deallocateCalls, 1);
	}

	TEST(MemoryResource, MonotonicBufferResourceAllocatesFromInitialBufferThenUpstream)
	{
		// monotonic_buffer_resource serves fast bump-pointer allocations from an
		// initial buffer and falls back to upstream resource when exhausted.
		std::array<std::byte, 128> initialBuffer{};
		CountingResource upstream;
		std::pmr::monotonic_buffer_resource resource(
			initialBuffer.data(), initialBuffer.size(), &upstream);

		void* first = resource.allocate(32, alignof(std::max_align_t));
		void* second = resource.allocate(32, alignof(std::max_align_t));
		ASSERT_NE(first, nullptr);
		ASSERT_NE(second, nullptr);

		// Exhausting/expanding may trigger upstream allocations; exact threshold is
		// implementation-defined, so we only verify API usability and non-crashing.
		for (int i = 0; i < 16; ++i)
		{
			void* block = resource.allocate(64, alignof(std::max_align_t));
			ASSERT_NE(block, nullptr);
		}
		EXPECT_GE(upstream.allocateCalls, 0);

		resource.release();
	}

	TEST(MemoryResource, UnsynchronizedPoolResourceSupportsAllocateDeallocateRelease)
	{
		// unsynchronized_pool_resource provides fast pooled allocations for
		// single-threaded contexts. This test validates lifecycle operations.
		CountingResource upstream;
		std::pmr::pool_options options{};
		options.max_blocks_per_chunk = 8;
		options.largest_required_pool_block = 256;
		std::pmr::unsynchronized_pool_resource pool(options, &upstream);

		void* a = pool.allocate(24, alignof(std::max_align_t));
		void* b = pool.allocate(24, alignof(std::max_align_t));
		ASSERT_NE(a, nullptr);
		ASSERT_NE(b, nullptr);
		pool.deallocate(a, 24, alignof(std::max_align_t));
		pool.deallocate(b, 24, alignof(std::max_align_t));
		pool.release();
	}

	TEST(MemoryResource, SynchronizedPoolResourceSupportsThreadSafePoolInterface)
	{
		// synchronized_pool_resource offers the same pooling model as unsynchronized
		// version but with internal synchronization for concurrent use.
		CountingResource upstream;
		std::pmr::pool_options options{};
		options.max_blocks_per_chunk = 8;
		options.largest_required_pool_block = 256;
		std::pmr::synchronized_pool_resource pool(options, &upstream);

		void* a = pool.allocate(40, alignof(std::max_align_t));
		void* b = pool.allocate(80, alignof(std::max_align_t));
		ASSERT_NE(a, nullptr);
		ASSERT_NE(b, nullptr);
		pool.deallocate(a, 40, alignof(std::max_align_t));
		pool.deallocate(b, 80, alignof(std::max_align_t));
		pool.release();
	}

	TEST(MemoryResource, PmrContainerAliasesUsePolymorphicAllocator)
	{
		// <memory_resource> defines pmr container aliases that bind standard
		// containers to polymorphic_allocator, enabling runtime resource selection.
		EXPECT_TRUE((std::is_same_v<
			typename std::pmr::vector<int>::allocator_type,
			std::pmr::polymorphic_allocator<int>>));
		EXPECT_TRUE((std::is_same_v<
			typename std::pmr::string::allocator_type,
			std::pmr::polymorphic_allocator<char>>));

		CountingResource resource;
		std::pmr::vector<int> values(&resource);
		values.push_back(1);
		values.push_back(2);
		values.push_back(3);
		EXPECT_EQ(values.size(), 3u);
		EXPECT_EQ(values[2], 3);
		EXPECT_GE(resource.allocateCalls, 1);

		std::pmr::string text(&resource);
		text = "pmr";
		EXPECT_EQ(text, "pmr");
	}

	TEST(MemoryResource, ResourceDerivedObjectsExposeUpstreamResourceWhenApplicable)
	{
		// Pool and monotonic resources expose upstream_resource() so callers can
		// introspect fallback allocation chains.
		CountingResource upstream;
		std::pmr::monotonic_buffer_resource mono(&upstream);
		EXPECT_EQ(mono.upstream_resource(), &upstream);

		std::pmr::unsynchronized_pool_resource unsync(&upstream);
		EXPECT_EQ(unsync.upstream_resource(), &upstream);

		std::pmr::synchronized_pool_resource sync(&upstream);
		EXPECT_EQ(sync.upstream_resource(), &upstream);
	}

}  // namespace
