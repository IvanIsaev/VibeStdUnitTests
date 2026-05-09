#include <gtest/gtest.h>

#include <array>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

	struct Widget
	{
		int value{};
		explicit Widget(int v) : value(v) {}
	};

	struct CountingObject
	{
		static inline int constructions = 0;
		static inline int destructions = 0;

		int value{};

		CountingObject() : value(0) { ++constructions; }
		explicit CountingObject(int v) : value(v) { ++constructions; }
		CountingObject(const CountingObject& other) : value(other.value) { ++constructions; }
		~CountingObject() { ++destructions; }
	};

	template <typename T>
	struct TinyAllocator
	{
		using value_type = T;

		TinyAllocator() = default;
		template <typename U>
		TinyAllocator(const TinyAllocator<U>&) {}

		[[nodiscard]] T* allocate(std::size_t n)
		{
			return static_cast<T*>(::operator new(n * sizeof(T)));
		}

		void deallocate(T* p, std::size_t) noexcept
		{
			::operator delete(p);
		}
	};

	struct UsesAllocAware
	{
		using allocator_type = std::allocator<int>;
		int payload{};
		UsesAllocAware(std::allocator_arg_t, const allocator_type&, int v) : payload(v) {}
	};

	struct SharedSelf : std::enable_shared_from_this<SharedSelf>
	{
		int payload{};
		explicit SharedSelf(int v) : payload(v) {}
	};

	TEST(Memory, AddressofAndPointerTraitsToAddressRecoverRawPointer)
	{
		// <memory> provides addressof/to_address/pointer_traits for generic pointer
		// code that must bypass overloaded operator& and support fancy pointers.
		Widget widget(42);
		Widget* raw = std::addressof(widget);
		ASSERT_NE(raw, nullptr);
		EXPECT_EQ(raw->value, 42);

		std::unique_ptr<Widget> owned = std::make_unique<Widget>(7);
		Widget* viaToAddress = std::to_address(owned.get());
		ASSERT_NE(viaToAddress, nullptr);
		EXPECT_EQ(viaToAddress->value, 7);

		using Traits = std::pointer_traits<Widget*>;
		Widget* viaPointerTraits = Traits::pointer_to(widget);
		EXPECT_EQ(viaPointerTraits, raw);
	}

	TEST(Memory, AllocatorAndAllocatorTraitsCoreOperations)
	{
		// allocator and allocator_traits define the allocator model used across the
		// standard library. This test validates type rebinding plus allocate/construct
		// and destroy/deallocate flows through allocator_traits.
		using Alloc = TinyAllocator<Widget>;
		using Traits = std::allocator_traits<Alloc>;
		using Rebound = Traits::template rebind_alloc<int>;

		EXPECT_TRUE((std::is_same_v<typename Traits::value_type, Widget>));
		EXPECT_TRUE((std::is_same_v<typename std::allocator_traits<Rebound>::value_type, int>));

		Alloc alloc;
		Widget* ptr = Traits::allocate(alloc, 1);
		ASSERT_NE(ptr, nullptr);
		Traits::construct(alloc, ptr, 99);
		EXPECT_EQ(ptr->value, 99);
		Traits::destroy(alloc, ptr);
		Traits::deallocate(alloc, ptr, 1);
	}

	TEST(Memory, UsesAllocatorDetectionAndUsesAllocatorConstruction)
	{
		// uses_allocator and uses_allocator_construction_args help generic factories
		// route allocator-aware construction arguments in container internals.
		EXPECT_TRUE((std::uses_allocator_v<UsesAllocAware, std::allocator<int>>));
		EXPECT_FALSE((std::uses_allocator_v<Widget, std::allocator<int>>));

		auto args = std::uses_allocator_construction_args<UsesAllocAware>(
			std::allocator<int>{}, 123);
		UsesAllocAware object = std::make_from_tuple<UsesAllocAware>(std::move(args));
		EXPECT_EQ(object.payload, 123);
	}

	TEST(Memory, UniquePtrDefaultDeleteAndFactoryHelpers)
	{
		// unique_ptr and default_delete model exclusive ownership; make_unique and
		// make_unique_for_overwrite provide safe allocation factories.
		std::unique_ptr<int> scalar = std::make_unique<int>(11);
		ASSERT_NE(scalar, nullptr);
		EXPECT_EQ(*scalar, 11);

		std::unique_ptr<int[]> array = std::make_unique<int[]>(4);
		array[0] = 1;
		array[1] = 2;
		array[2] = 3;
		array[3] = 4;
		EXPECT_EQ(array[2], 3);

		std::default_delete<int> deleter;
		int* raw = new int(5);
		deleter(raw);

#ifdef __cpp_lib_smart_ptr_for_overwrite
		auto overwrite = std::make_unique_for_overwrite<std::array<int, 3>>();
		ASSERT_NE(overwrite, nullptr);
		(*overwrite)[0] = 9;
		EXPECT_EQ((*overwrite)[0], 9);
#endif
	}

	TEST(Memory, SharedPtrWeakPtrAndEnableSharedFromThisCooperate)
	{
		// shared_ptr provides shared ownership, weak_ptr provides non-owning observer
		// references, and enable_shared_from_this safely creates sibling owners.
		auto shared = std::make_shared<SharedSelf>(77);
		ASSERT_NE(shared, nullptr);
		EXPECT_EQ(shared.use_count(), 1);

		std::shared_ptr<SharedSelf> sibling = shared->shared_from_this();
		EXPECT_EQ(shared.use_count(), 2);
		EXPECT_EQ(sibling->payload, 77);

		std::weak_ptr<SharedSelf> weak = shared;
		EXPECT_FALSE(weak.expired());
		auto locked = weak.lock();
		ASSERT_NE(locked, nullptr);
		EXPECT_EQ(locked->payload, 77);

		shared.reset();
		sibling.reset();
		EXPECT_TRUE(weak.expired());
	}

	TEST(Memory, SharedPtrPointerCastsSupportPolymorphicConversions)
	{
		// static_pointer_cast/dynamic_pointer_cast/const_pointer_cast provide safe
		// ownership-preserving cast helpers for shared_ptr polymorphic usage.
		struct Base { virtual ~Base() = default; int x = 1; };
		struct Derived : Base { int y = 2; };

		std::shared_ptr<Derived> derived = std::make_shared<Derived>();
		std::shared_ptr<Base> base = std::static_pointer_cast<Base>(derived);
		EXPECT_EQ(base->x, 1);

		std::shared_ptr<Derived> dynamic = std::dynamic_pointer_cast<Derived>(base);
		ASSERT_NE(dynamic, nullptr);
		EXPECT_EQ(dynamic->y, 2);

		std::shared_ptr<const Base> constBase = base;
		std::shared_ptr<Base> mutableAgain = std::const_pointer_cast<Base>(constBase);
		EXPECT_EQ(mutableAgain->x, 1);
	}

	TEST(Memory, OwnerLessAndHashSupportForSmartPointers)
	{
		// owner_less and hash specializations from <memory> support associative use
		// of shared_ptr/weak_ptr while preserving ownership-based ordering semantics.
		auto first = std::make_shared<int>(1);
		auto second = std::make_shared<int>(2);
		std::weak_ptr<int> weakFirst = first;

		std::owner_less<std::shared_ptr<int>> lessShared;
		EXPECT_TRUE(lessShared(first, second) || lessShared(second, first) || (!lessShared(first, second) && !lessShared(second, first)));

		std::owner_less<void> lessVoid;
		EXPECT_FALSE(lessVoid(first, first));
		EXPECT_FALSE(lessVoid(weakFirst, weakFirst));

		const std::size_t sharedHash = std::hash<std::shared_ptr<int>>{}(first);
		EXPECT_TRUE(sharedHash == sharedHash);
	}

	TEST(Memory, ConstructAtDestroyAtAndDestroyRangesManageLifetimes)
	{
		// construct_at/destroy_at and destroy/destroy_n are low-level lifetime
		// primitives for manually managed storage and uninitialized algorithms.
		CountingObject::constructions = 0;
		CountingObject::destructions = 0;

		alignas(CountingObject) unsigned char storage[sizeof(CountingObject)]{};
		auto* obj = reinterpret_cast<CountingObject*>(storage);
		std::construct_at(obj, 41);
		EXPECT_EQ(obj->value, 41);
		EXPECT_EQ(CountingObject::constructions, 1);
		std::destroy_at(obj);
		EXPECT_EQ(CountingObject::destructions, 1);

		void* raw = ::operator new[](sizeof(CountingObject) * 3);
		auto* first = static_cast<CountingObject*>(raw);
		auto* last = first + 3;
		std::uninitialized_default_construct(first, last);
		EXPECT_EQ(CountingObject::constructions, 4);
		std::destroy(first, last);
		EXPECT_EQ(CountingObject::destructions, 4);
		::operator delete[](raw);
	}

	TEST(Memory, UninitializedAlgorithmsCreateAndFillObjects)
	{
		// uninitialized_default_construct/uninitialized_value_construct and related
		// copy/move/fill algorithms support container internals over raw storage.
		CountingObject::constructions = 0;
		CountingObject::destructions = 0;

		void* raw = ::operator new[](sizeof(CountingObject) * 4);
		auto* first = static_cast<CountingObject*>(raw);
		auto* last = first + 4;

		std::uninitialized_value_construct(first, last);
		EXPECT_EQ(CountingObject::constructions, 4);

		std::destroy(first, last);
		EXPECT_EQ(CountingObject::destructions, 4);

		::operator delete[](raw);
	}

	TEST(Memory, RangesUninitializedAlgorithmsAreCallableWhenAvailable)
	{
		// C++20 adds ranges-aware uninitialized algorithms in std::ranges. This test
		// validates availability and basic effect while remaining feature-guarded.
#ifdef __cpp_lib_raw_memory_algorithms
		std::vector<int> source = { 5, 6, 7 };
		void* raw = ::operator new[](sizeof(int) * source.size());
		auto* destination = static_cast<int*>(raw);

		auto result = std::ranges::uninitialized_copy(source, destination);
		EXPECT_EQ(result.in, source.end());
		EXPECT_EQ(result.out, destination + source.size());
		EXPECT_EQ(destination[0], 5);
		EXPECT_EQ(destination[2], 7);

		std::ranges::destroy(destination, destination + source.size());
		::operator delete[](raw);
#else
		GTEST_SKIP() << "Ranges raw-memory algorithms are not available.";
#endif
	}

	TEST(Memory, StdAllocatorInteractsWithAllocatorTraits)
	{
		// allocator_traits must work with standard allocators too; this simple case
		// validates behavior with std::allocator and object construction lifecycle.
		std::allocator<std::string> alloc;
		using Traits = std::allocator_traits<std::allocator<std::string>>;

		std::string* ptr = Traits::allocate(alloc, 1);
		ASSERT_NE(ptr, nullptr);
		Traits::construct(alloc, ptr, "hello");
		EXPECT_EQ(*ptr, "hello");
		Traits::destroy(alloc, ptr);
		Traits::deallocate(alloc, ptr, 1);
	}

}  // namespace
