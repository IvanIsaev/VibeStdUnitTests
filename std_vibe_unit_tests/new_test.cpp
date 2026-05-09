#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <type_traits>

namespace {

	struct alignas(64) OverAlignedType
	{
		std::uint64_t payload[4];
	};

	struct ConstructProbe
	{
		int value;
		explicit ConstructProbe(int v) : value(v) {}
		~ConstructProbe() = default;
	};

	TEST(NewHeader, NewHandlerSetAndGetRoundTrip)
	{
		// <new> exposes new_handler plus set_new_handler/get_new_handler to control
		// out-of-memory handling for throwing allocations. We validate installation
		// and restoration behavior without triggering allocation failure.
		auto original = std::get_new_handler();
		auto replacement = +[]() { throw std::bad_alloc(); };

		auto previous = std::set_new_handler(replacement);
		EXPECT_EQ(previous, original);
		EXPECT_EQ(std::get_new_handler(), replacement);

		auto restorePrevious = std::set_new_handler(original);
		EXPECT_EQ(restorePrevious, replacement);
		EXPECT_EQ(std::get_new_handler(), original);
	}

	TEST(NewHeader, BadAllocHierarchyAndWhatMessage)
	{
		// std::bad_alloc is the base allocation failure exception and derives from
		// std::exception. bad_array_new_length is a specialized allocation exception
		// type for invalid array-new length scenarios.
		EXPECT_TRUE((std::is_base_of_v<std::exception, std::bad_alloc>));
		EXPECT_TRUE((std::is_base_of_v<std::bad_alloc, std::bad_array_new_length>));

		const std::bad_alloc bad;
		const std::bad_array_new_length badLen;
		EXPECT_NE(*bad.what(), '\0');
		EXPECT_NE(*badLen.what(), '\0');
	}

	TEST(NewHeader, NothrowTagTypeAndInstanceAreUsable)
	{
		// std::nothrow_t and the global std::nothrow object select non-throwing
		// allocation forms. This test validates the tag type and performs a basic
		// nothrow allocation/deallocation round trip.
		EXPECT_TRUE((std::is_class_v<std::nothrow_t>));

		void* memory = ::operator new(32u, std::nothrow);
		ASSERT_NE(memory, nullptr);
		::operator delete(memory, std::nothrow);
	}

	TEST(NewHeader, ThrowingOperatorNewAndDeleteWorkForScalarStorage)
	{
		// Global throwing operator new/operator delete allocate and release raw
		// storage. This verifies the basic scalar forms from <new> and confirms that
		// allocated storage can be addressed as bytes.
		void* memory = ::operator new(64u);
		ASSERT_NE(memory, nullptr);

		auto* bytes = static_cast<unsigned char*>(memory);
		bytes[0] = 0xAB;
		EXPECT_EQ(bytes[0], 0xAB);

		::operator delete(memory);
	}

	TEST(NewHeader, ThrowingArrayOperatorNewAndDeleteWork)
	{
		// <new> also declares array forms of global allocation functions. Calling
		// these directly exercises operator new[] and operator delete[] contracts.
		void* memory = ::operator new[](48u);
		ASSERT_NE(memory, nullptr);

		auto* bytes = static_cast<unsigned char*>(memory);
		bytes[0] = 0x5A;
		EXPECT_EQ(bytes[0], 0x5A);

		::operator delete[](memory);
	}

	TEST(NewHeader, PlacementNewTagConstructsObjectInProvidedBuffer)
	{
		// placement new uses the (size_t, void*) overload and constructs an object
		// into caller-supplied storage. The placement delete counterpart is a no-op
		// form used only for matching signatures in exceptional construction paths.
		alignas(ConstructProbe) unsigned char storage[sizeof(ConstructProbe)]{};

		auto* probe = new (storage) ConstructProbe(42);
		ASSERT_NE(probe, nullptr);
		EXPECT_EQ(probe->value, 42);

		probe->~ConstructProbe();
	}

	TEST(NewHeader, AlignedAllocationFormsRespectRequestedAlignment)
	{
		// Since C++17, align_val_t-tagged allocation forms allow requesting specific
		// dynamic alignment. This test allocates over-aligned storage and validates
		// pointer alignment before releasing it with the matching aligned delete.
		const std::align_val_t alignment = std::align_val_t{ alignof(OverAlignedType) };
		void* memory = ::operator new(sizeof(OverAlignedType), alignment);
		ASSERT_NE(memory, nullptr);

		const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(memory);
		EXPECT_EQ(address % alignof(OverAlignedType), 0u);

		::operator delete(memory, alignment);
	}

	TEST(NewHeader, AlignedNothrowAndArrayAllocationFormsAreUsable)
	{
		// <new> provides aligned + nothrow forms and aligned array forms. We test
		// both APIs and free memory with corresponding delete overload signatures.
		const std::align_val_t alignment = std::align_val_t{ alignof(OverAlignedType) };

		void* scalar = ::operator new(sizeof(OverAlignedType), alignment, std::nothrow);
		ASSERT_NE(scalar, nullptr);
		::operator delete(scalar, alignment, std::nothrow);

		void* arrayStorage = ::operator new[](2u * sizeof(OverAlignedType), alignment);
		ASSERT_NE(arrayStorage, nullptr);
		::operator delete[](arrayStorage, alignment);
	}

	TEST(NewHeader, SizedDeleteOverloadsAreCallable)
	{
		// Sized deallocation overloads accept the originally requested size. While
		// not all implementations always use size information internally, the public
		// overload set must be callable and paired correctly with allocations.
		void* memory = ::operator new(80u);
		ASSERT_NE(memory, nullptr);
		::operator delete(memory, std::size_t{ 80u });

		const std::align_val_t alignment = std::align_val_t{ alignof(OverAlignedType) };
		void* alignedMemory = ::operator new(sizeof(OverAlignedType), alignment);
		ASSERT_NE(alignedMemory, nullptr);
		::operator delete(alignedMemory, std::size_t{ sizeof(OverAlignedType) }, alignment);
	}

	TEST(NewHeader, LaunderReturnsPointerToCurrentObject)
	{
		// std::launder is used after creating/reusing objects in storage where strict
		// aliasing/lifetime rules require obtaining a fresh pointer value. For a live
		// object pointer, launder should return an equivalent usable pointer.
		int value = 7;
		int* ptr = &value;
		int* laundered = std::launder(ptr);
		ASSERT_NE(laundered, nullptr);
		EXPECT_EQ(*laundered, 7);
		EXPECT_EQ(laundered, ptr);
	}

	TEST(NewHeader, HardwareInterferenceSizeConstantsAreSensible)
	{
		// hardware_destructive_interference_size and
		// hardware_constructive_interference_size model cache-line related layout
		// guidance. They are nonzero integral constants suitable for alignas usage.
		EXPECT_GE(std::hardware_destructive_interference_size, 1u);
		EXPECT_GE(std::hardware_constructive_interference_size, 1u);
	}

}  // namespace
