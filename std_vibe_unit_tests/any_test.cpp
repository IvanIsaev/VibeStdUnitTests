#include <gtest/gtest.h>

#include <any>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

	struct MoveOnly
	{
		int value = 0;
		explicit MoveOnly(int v) : value(v) {}
		MoveOnly(MoveOnly&&) noexcept = default;
		MoveOnly& operator=(MoveOnly&&) noexcept = default;
		MoveOnly(const MoveOnly&) = delete;
		MoveOnly& operator=(const MoveOnly&) = delete;
	};

	struct Tracker
	{
		static inline int constructions = 0;
		static inline int destructions = 0;
		int payload = 0;
		explicit Tracker(int v = 0) : payload(v) { ++constructions; }
		Tracker(const Tracker& other) : payload(other.payload) { ++constructions; }
		Tracker(Tracker&& other) noexcept : payload(other.payload) { ++constructions; other.payload = -1; }
		~Tracker() { ++destructions; }
	};

	TEST(AnyHeader, AnyDefaultConstructionHasNoValueAndReportsVoidType)
	{
		// std::any default-constructs to an empty state. In that state, has_value()
		// is false and type() reports typeid(void), which is the standardized marker
		// used by observers to describe "no contained object."
		std::any value;
		EXPECT_FALSE(value.has_value());
		EXPECT_EQ(value.type(), typeid(void));
	}

	TEST(AnyHeader, ValueConstructionCopyMoveAndAssignmentPreserveContainedType)
	{
		// std::any can hold any copy-constructible type and supports copy/move
		// semantics. This test validates assignment paths and type/value observers.
		std::any value = 42;
		EXPECT_TRUE(value.has_value());
		EXPECT_EQ(value.type(), typeid(int));
		EXPECT_EQ(std::any_cast<int>(value), 42);

		value = std::string("hello");
		EXPECT_EQ(value.type(), typeid(std::string));
		EXPECT_EQ(std::any_cast<std::string>(value), "hello");

		std::any copied = value;
		EXPECT_EQ(copied.type(), typeid(std::string));
		EXPECT_EQ(std::any_cast<std::string>(copied), "hello");

		std::any moved = std::move(copied);
		EXPECT_EQ(moved.type(), typeid(std::string));
		EXPECT_EQ(std::any_cast<std::string>(moved), "hello");
	}

	TEST(AnyHeader, ResetClearsStoredObjectAndRunsDestructor)
	{
		// reset() destroys the currently held object (if any) and returns the any to
		// empty state. We track construction/destruction counters to verify cleanup.
		Tracker::constructions = 0;
		Tracker::destructions = 0;

		std::any value = Tracker(7);
		EXPECT_TRUE(value.has_value());
		EXPECT_GE(Tracker::constructions, 1);

		value.reset();
		EXPECT_FALSE(value.has_value());
		EXPECT_EQ(value.type(), typeid(void));
		EXPECT_GE(Tracker::destructions, 1);
	}

	TEST(AnyHeader, SwapExchangesContainedValuesAndTypes)
	{
		// swap exchanges full any states, including both contained type and value.
		// This supports efficient reordering and transactional replacement patterns.
		std::any first = 10;
		std::any second = std::string("world");

		first.swap(second);
		EXPECT_EQ(first.type(), typeid(std::string));
		EXPECT_EQ(second.type(), typeid(int));
		EXPECT_EQ(std::any_cast<std::string>(first), "world");
		EXPECT_EQ(std::any_cast<int>(second), 10);

		using std::swap;
		swap(first, second);
		EXPECT_EQ(first.type(), typeid(int));
		EXPECT_EQ(second.type(), typeid(std::string));
	}

	TEST(AnyHeader, EmplaceConstructsInPlaceAndReturnsReference)
	{
		// emplace<T>(args...) destroys existing content and constructs T in-place.
		// It returns a reference to the newly created object for immediate mutation.
		std::any value;
		std::string& ref = value.emplace<std::string>(3, 'a');
		EXPECT_EQ(ref, "aaa");
		ref.push_back('!');
		EXPECT_EQ(std::any_cast<std::string>(value), "aaa!");

		auto& vecRef = value.emplace<std::vector<int>>(5, 9);
		EXPECT_EQ(vecRef.size(), 5u);
		EXPECT_EQ(vecRef[0], 9);
		EXPECT_EQ(value.type(), typeid(std::vector<int>));
	}

	TEST(AnyHeader, MakeAnyConstructsAnyWithRequestedType)
	{
		// make_any<T>(args...) is a factory helper mirroring make_unique/make_shared
		// style APIs, constructing T directly inside a returned std::any.
		std::any text = std::make_any<std::string>("vibe");
		EXPECT_EQ(text.type(), typeid(std::string));
		EXPECT_EQ(std::any_cast<std::string>(text), "vibe");

		std::any numbers = std::make_any<std::vector<int>>(4, 3);
		EXPECT_EQ(numbers.type(), typeid(std::vector<int>));
		EXPECT_EQ(std::any_cast<std::vector<int>>(numbers).size(), 4u);
	}

	TEST(AnyHeader, AnyCastValueReferenceAndPointerOverloads)
	{
		// any_cast supports value, reference, and pointer forms:
		// - value/reference forms throw bad_any_cast on mismatch
		// - pointer forms return nullptr on mismatch
		std::any value = std::string("alpha");

		const std::string copied = std::any_cast<std::string>(value);
		EXPECT_EQ(copied, "alpha");

		std::string& byRef = std::any_cast<std::string&>(value);
		byRef += "-beta";
		EXPECT_EQ(std::any_cast<std::string>(value), "alpha-beta");

		const std::any& constValue = value;
		const std::string& constRef = std::any_cast<const std::string&>(constValue);
		EXPECT_EQ(constRef, "alpha-beta");

		std::string* ptr = std::any_cast<std::string>(&value);
		ASSERT_NE(ptr, nullptr);
		EXPECT_EQ(*ptr, "alpha-beta");

		const std::string* constPtr = std::any_cast<std::string>(&constValue);
		ASSERT_NE(constPtr, nullptr);
		EXPECT_EQ(*constPtr, "alpha-beta");

		int* wrongPtr = std::any_cast<int>(&value);
		EXPECT_EQ(wrongPtr, nullptr);
	}

	TEST(AnyHeader, BadAnyCastIsThrownOnTypeMismatch)
	{
		// When requested type does not match stored type, value/reference any_cast
		// throws std::bad_any_cast. The exception derives from std::bad_cast.
		std::any value = 123;

		EXPECT_THROW((void)std::any_cast<std::string>(value), std::bad_any_cast);
		EXPECT_THROW((void)std::any_cast<double&>(value), std::bad_any_cast);

		try
		{
			(void)std::any_cast<std::string>(value);
			FAIL() << "Expected std::bad_any_cast.";
		}
		catch (const std::bad_any_cast& ex)
		{
			EXPECT_NE(*ex.what(), '\0');
			EXPECT_TRUE((std::is_base_of_v<std::bad_cast, std::bad_any_cast>));
		}
		catch (...)
		{
			FAIL() << "Unexpected exception type.";
		}
	}

	TEST(AnyHeader, MoveOnlyTypeCanBeEmplacedAndRetrievedByReference)
	{
		// std::any requires copy-constructible type for direct value construction,
		// but move-only types can still be created via emplace and accessed by ref.
		std::any value;
		MoveOnly& obj = value.emplace<MoveOnly>(55);
		EXPECT_EQ(obj.value, 55);

		MoveOnly& retrieved = std::any_cast<MoveOnly&>(value);
		EXPECT_EQ(retrieved.value, 55);
	}

}  // namespace
