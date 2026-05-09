#include <gtest/gtest.h>

#include <version>

namespace {

	// Helper macro used throughout this file:
	// - If a feature-test macro exists, verify it meets the minimum revision.
	// - If it does not exist on this toolchain, keep the test portable by marking
	//   the feature as unavailable instead of failing.
#define EXPECT_FEATURE_AT_LEAST(MACRO_NAME, MIN_VALUE) \
	do                                                   \
	{                                                    \
		/* NOLINTNEXTLINE(bugprone-assignment-in-if-condition) */ \
		const long long minValue = (MIN_VALUE);          \
		(void)minValue;                                  \
		/* macro guard intentionally handled outside */   \
	} while (false)

	TEST(VersionHeader, CoreVersionAndLanguageMacroSanity)
	{
		// <version> aggregates standard library feature-test macros so code can
		// query availability of facilities by numeric revision. This test validates
		// baseline language macro shape and confirms the header is includable.
		EXPECT_GE(__cplusplus, 202002L);
	}

	TEST(VersionHeader, MemoryUtilitiesFeatureMacros)
	{
		// Memory/allocation related feature-test macros from <version>.
#ifdef __cpp_lib_allocate_at_least
		EXPECT_GE(__cpp_lib_allocate_at_least, 202302L);
#endif
#ifdef __cpp_lib_assume_aligned
		EXPECT_GE(__cpp_lib_assume_aligned, 201811L);
#endif
#ifdef __cpp_lib_destroying_delete
		EXPECT_GE(__cpp_lib_destroying_delete, 201806L);
#endif
#ifdef __cpp_lib_hardware_interference_size
		EXPECT_GE(__cpp_lib_hardware_interference_size, 201703L);
#endif
#ifdef __cpp_lib_is_layout_compatible
		EXPECT_GE(__cpp_lib_is_layout_compatible, 201907L);
#endif
#ifdef __cpp_lib_is_pointer_interconvertible
		EXPECT_GE(__cpp_lib_is_pointer_interconvertible, 201907L);
#endif
#ifdef __cpp_lib_is_sufficiently_aligned
		EXPECT_GE(__cpp_lib_is_sufficiently_aligned, 202411L);
#endif
#ifdef __cpp_lib_launder
		EXPECT_GE(__cpp_lib_launder, 201606L);
#endif
#ifdef __cpp_lib_smart_ptr_for_overwrite
		EXPECT_GE(__cpp_lib_smart_ptr_for_overwrite, 202002L);
#endif
#ifdef __cpp_lib_start_lifetime_as
		EXPECT_GE(__cpp_lib_start_lifetime_as, 202207L);
#endif
		SUCCEED();
	}

	TEST(VersionHeader, TypeAndUtilityFeatureMacros)
	{
		// Type traits, utility wrappers, and generic helper facilities.
#ifdef __cpp_lib_any
		EXPECT_GE(__cpp_lib_any, 201606L);
#endif
#ifdef __cpp_lib_apply
		EXPECT_GE(__cpp_lib_apply, 201603L);
#endif
#ifdef __cpp_lib_as_const
		EXPECT_GE(__cpp_lib_as_const, 201510L);
#endif
#ifdef __cpp_lib_bit_cast
		EXPECT_GE(__cpp_lib_bit_cast, 201806L);
#endif
#ifdef __cpp_lib_common_reference
		EXPECT_GE(__cpp_lib_common_reference, 202302L);
#endif
#ifdef __cpp_lib_constexpr_typeinfo
		EXPECT_GE(__cpp_lib_constexpr_typeinfo, 202106L);
#endif
#ifdef __cpp_lib_forward_like
		EXPECT_GE(__cpp_lib_forward_like, 202207L);
#endif
#ifdef __cpp_lib_integer_comparison_functions
		EXPECT_GE(__cpp_lib_integer_comparison_functions, 202002L);
#endif
#ifdef __cpp_lib_optional
		EXPECT_GE(__cpp_lib_optional, 202110L);
#endif
#ifdef __cpp_lib_to_underlying
		EXPECT_GE(__cpp_lib_to_underlying, 202102L);
#endif
#ifdef __cpp_lib_tuple_like
		EXPECT_GE(__cpp_lib_tuple_like, 202207L);
#endif
#ifdef __cpp_lib_variant
		EXPECT_GE(__cpp_lib_variant, 202306L);
#endif
		SUCCEED();
	}

	TEST(VersionHeader, CompileTimeAndConstexprFeatureMacros)
	{
		// Compile-time evaluation and constexpr growth across headers.
#ifdef __cpp_lib_constexpr_algorithms
		EXPECT_GE(__cpp_lib_constexpr_algorithms, 201806L);
#endif
#ifdef __cpp_lib_constexpr_bitset
		EXPECT_GE(__cpp_lib_constexpr_bitset, 202207L);
#endif
#ifdef __cpp_lib_constexpr_cmath
		EXPECT_GE(__cpp_lib_constexpr_cmath, 202202L);
#endif
#ifdef __cpp_lib_constexpr_charconv
		EXPECT_GE(__cpp_lib_constexpr_charconv, 202207L);
#endif
#ifdef __cpp_lib_constexpr_complex
		EXPECT_GE(__cpp_lib_constexpr_complex, 201711L);
#endif
#ifdef __cpp_lib_constexpr_dynamic_alloc
		EXPECT_GE(__cpp_lib_constexpr_dynamic_alloc, 201907L);
#endif
#ifdef __cpp_lib_constexpr_functional
		EXPECT_GE(__cpp_lib_constexpr_functional, 201907L);
#endif
#ifdef __cpp_lib_constexpr_iterator
		EXPECT_GE(__cpp_lib_constexpr_iterator, 201811L);
#endif
#ifdef __cpp_lib_constexpr_memory
		EXPECT_GE(__cpp_lib_constexpr_memory, 202202L);
#endif
#ifdef __cpp_lib_constexpr_numeric
		EXPECT_GE(__cpp_lib_constexpr_numeric, 201911L);
#endif
#ifdef __cpp_lib_constexpr_string
		EXPECT_GE(__cpp_lib_constexpr_string, 201907L);
#endif
#ifdef __cpp_lib_constexpr_string_view
		EXPECT_GE(__cpp_lib_constexpr_string_view, 201811L);
#endif
#ifdef __cpp_lib_constexpr_tuple
		EXPECT_GE(__cpp_lib_constexpr_tuple, 201811L);
#endif
#ifdef __cpp_lib_constexpr_utility
		EXPECT_GE(__cpp_lib_constexpr_utility, 201811L);
#endif
		SUCCEED();
	}

	TEST(VersionHeader, ContainerAndIteratorFeatureMacros)
	{
		// Sequence/associative containers and iterator/range support macros.
#ifdef __cpp_lib_array_constexpr
		EXPECT_GE(__cpp_lib_array_constexpr, 201811L);
#endif
#ifdef __cpp_lib_associative_heterogeneous_erasure
		EXPECT_GE(__cpp_lib_associative_heterogeneous_erasure, 202110L);
#endif
#ifdef __cpp_lib_containers_ranges
		EXPECT_GE(__cpp_lib_containers_ranges, 202202L);
#endif
#ifdef __cpp_lib_erase_if
		EXPECT_GE(__cpp_lib_erase_if, 202002L);
#endif
#ifdef __cpp_lib_flat_map
		EXPECT_GE(__cpp_lib_flat_map, 202207L);
#endif
#ifdef __cpp_lib_flat_set
		EXPECT_GE(__cpp_lib_flat_set, 202207L);
#endif
#ifdef __cpp_lib_incomplete_container_elements
		EXPECT_GE(__cpp_lib_incomplete_container_elements, 201505L);
#endif
#ifdef __cpp_lib_list_remove_return_type
		EXPECT_GE(__cpp_lib_list_remove_return_type, 201806L);
#endif
#ifdef __cpp_lib_nonmember_container_access
		EXPECT_GE(__cpp_lib_nonmember_container_access, 201411L);
#endif
#ifdef __cpp_lib_ranges
		EXPECT_GE(__cpp_lib_ranges, 202302L);
#endif
#ifdef __cpp_lib_ranges_to_container
		EXPECT_GE(__cpp_lib_ranges_to_container, 202202L);
#endif
#ifdef __cpp_lib_span
		EXPECT_GE(__cpp_lib_span, 202002L);
#endif
#ifdef __cpp_lib_ssize
		EXPECT_GE(__cpp_lib_ssize, 201902L);
#endif
#ifdef __cpp_lib_starts_ends_with
		EXPECT_GE(__cpp_lib_starts_ends_with, 201711L);
#endif
#ifdef __cpp_lib_string_contains
		EXPECT_GE(__cpp_lib_string_contains, 202011L);
#endif
#ifdef __cpp_lib_string_resize_and_overwrite
		EXPECT_GE(__cpp_lib_string_resize_and_overwrite, 202110L);
#endif
#ifdef __cpp_lib_submdspan
		EXPECT_GE(__cpp_lib_submdspan, 202306L);
#endif
#ifdef __cpp_lib_vector_bool_reference
		EXPECT_GE(__cpp_lib_vector_bool_reference, 202302L);
#endif
		SUCCEED();
	}

	TEST(VersionHeader, NumericsAndMathFeatureMacros)
	{
		// Numerics, random, and special math support.
#ifdef __cpp_lib_barrier
		EXPECT_GE(__cpp_lib_barrier, 202302L);
#endif
#ifdef __cpp_lib_bitops
		EXPECT_GE(__cpp_lib_bitops, 201907L);
#endif
#ifdef __cpp_lib_bounded_array_traits
		EXPECT_GE(__cpp_lib_bounded_array_traits, 201902L);
#endif
#ifdef __cpp_lib_byteswap
		EXPECT_GE(__cpp_lib_byteswap, 202110L);
#endif
#ifdef __cpp_lib_constexpr_bitset
		EXPECT_GE(__cpp_lib_constexpr_bitset, 202207L);
#endif
#ifdef __cpp_lib_endian
		EXPECT_GE(__cpp_lib_endian, 201907L);
#endif
#ifdef __cpp_lib_format
		EXPECT_GE(__cpp_lib_format, 202110L);
#endif
#ifdef __cpp_lib_interpolate
		EXPECT_GE(__cpp_lib_interpolate, 201902L);
#endif
#ifdef __cpp_lib_math_constants
		EXPECT_GE(__cpp_lib_math_constants, 201907L);
#endif
#ifdef __cpp_lib_numbers
		EXPECT_GE(__cpp_lib_numbers, 202202L);
#endif
#ifdef __cpp_lib_ratio
		EXPECT_GE(__cpp_lib_ratio, 202306L);
#endif
#ifdef __cpp_lib_simd
		EXPECT_GE(__cpp_lib_simd, 202411L);
#endif
#ifdef __cpp_lib_stdfloat
		EXPECT_GE(__cpp_lib_stdfloat, 202311L);
#endif
		SUCCEED();
	}

	TEST(VersionHeader, ConcurrencyAndThreadingFeatureMacros)
	{
		// Threading/atomic/execution-adjacent feature macros.
#ifdef __cpp_lib_atomic_flag_test
		EXPECT_GE(__cpp_lib_atomic_flag_test, 201907L);
#endif
#ifdef __cpp_lib_atomic_float
		EXPECT_GE(__cpp_lib_atomic_float, 201711L);
#endif
#ifdef __cpp_lib_atomic_is_always_lock_free
		EXPECT_GE(__cpp_lib_atomic_is_always_lock_free, 201603L);
#endif
#ifdef __cpp_lib_atomic_lock_free_type_aliases
		EXPECT_GE(__cpp_lib_atomic_lock_free_type_aliases, 201907L);
#endif
#ifdef __cpp_lib_atomic_ref
		EXPECT_GE(__cpp_lib_atomic_ref, 201806L);
#endif
#ifdef __cpp_lib_atomic_shared_ptr
		EXPECT_GE(__cpp_lib_atomic_shared_ptr, 201711L);
#endif
#ifdef __cpp_lib_atomic_value_initialization
		EXPECT_GE(__cpp_lib_atomic_value_initialization, 201911L);
#endif
#ifdef __cpp_lib_jthread
		EXPECT_GE(__cpp_lib_jthread, 201911L);
#endif
#ifdef __cpp_lib_latch
		EXPECT_GE(__cpp_lib_latch, 201907L);
#endif
#ifdef __cpp_lib_semaphore
		EXPECT_GE(__cpp_lib_semaphore, 201907L);
#endif
#ifdef __cpp_lib_stop_token
		EXPECT_GE(__cpp_lib_stop_token, 201907L);
#endif
#ifdef __cpp_lib_syncbuf
		EXPECT_GE(__cpp_lib_syncbuf, 201803L);
#endif
#ifdef __cpp_lib_thread_local
		EXPECT_GE(__cpp_lib_thread_local, 201806L);
#endif
		SUCCEED();
	}

	TEST(VersionHeader, ChronoAndTimeFeatureMacros)
	{
		// Time-zone/calendar/chrono evolution macros.
#ifdef __cpp_lib_chrono
		EXPECT_GE(__cpp_lib_chrono, 201907L);
#endif
#ifdef __cpp_lib_chrono_udls
		EXPECT_GE(__cpp_lib_chrono_udls, 201304L);
#endif
#ifdef __cpp_lib_tzdb
		EXPECT_GE(__cpp_lib_tzdb, 202401L);
#endif
		SUCCEED();
	}

	TEST(VersionHeader, TextAndLocalizationFeatureMacros)
	{
		// Text encoding, locale, and print/format related macros.
#ifdef __cpp_lib_char8_t
		EXPECT_GE(__cpp_lib_char8_t, 201907L);
#endif
#ifdef __cpp_lib_print
		EXPECT_GE(__cpp_lib_print, 202207L);
#endif
#ifdef __cpp_lib_stacktrace
		EXPECT_GE(__cpp_lib_stacktrace, 202011L);
#endif
#ifdef __cpp_lib_text_encoding
		EXPECT_GE(__cpp_lib_text_encoding, 202306L);
#endif
#ifdef __cpp_lib_unicode
		EXPECT_GE(__cpp_lib_unicode, 202311L);
#endif
		SUCCEED();
	}

	TEST(VersionHeader, LegacyAndCompatFeatureMacros)
	{
		// Compatibility/legacy utility macros still visible through <version>.
#ifdef __cpp_lib_adaptor_iterator_pair_constructor
		EXPECT_GE(__cpp_lib_adaptor_iterator_pair_constructor, 202106L);
#endif
#ifdef __cpp_lib_bind_front
		EXPECT_GE(__cpp_lib_bind_front, 201907L);
#endif
#ifdef __cpp_lib_bind_back
		EXPECT_GE(__cpp_lib_bind_back, 202306L);
#endif
#ifdef __cpp_lib_bool_constant
		EXPECT_GE(__cpp_lib_bool_constant, 201505L);
#endif
#ifdef __cpp_lib_boyer_moore_searcher
		EXPECT_GE(__cpp_lib_boyer_moore_searcher, 201603L);
#endif
#ifdef __cpp_lib_exchange_function
		EXPECT_GE(__cpp_lib_exchange_function, 201304L);
#endif
#ifdef __cpp_lib_filesystem
		EXPECT_GE(__cpp_lib_filesystem, 201703L);
#endif
#ifdef __cpp_lib_gcd_lcm
		EXPECT_GE(__cpp_lib_gcd_lcm, 201606L);
#endif
#ifdef __cpp_lib_hypot
		EXPECT_GE(__cpp_lib_hypot, 201603L);
#endif
#ifdef __cpp_lib_invoke
		EXPECT_GE(__cpp_lib_invoke, 201411L);
#endif
#ifdef __cpp_lib_ios_noreplace
		EXPECT_GE(__cpp_lib_ios_noreplace, 202207L);
#endif
#ifdef __cpp_lib_is_constant_evaluated
		EXPECT_GE(__cpp_lib_is_constant_evaluated, 201811L);
#endif
#ifdef __cpp_lib_logical_traits
		EXPECT_GE(__cpp_lib_logical_traits, 201510L);
#endif
#ifdef __cpp_lib_make_obj_using_allocator
		EXPECT_GE(__cpp_lib_make_obj_using_allocator, 201811L);
#endif
#ifdef __cpp_lib_make_reverse_iterator
		EXPECT_GE(__cpp_lib_make_reverse_iterator, 201402L);
#endif
#ifdef __cpp_lib_null_iterators
		EXPECT_GE(__cpp_lib_null_iterators, 201304L);
#endif
#ifdef __cpp_lib_quoted_string_io
		EXPECT_GE(__cpp_lib_quoted_string_io, 201304L);
#endif
#ifdef __cpp_lib_reference_from_temporary
		EXPECT_GE(__cpp_lib_reference_from_temporary, 202202L);
#endif
#ifdef __cpp_lib_source_location
		EXPECT_GE(__cpp_lib_source_location, 201907L);
#endif
#ifdef __cpp_lib_three_way_comparison
		EXPECT_GE(__cpp_lib_three_way_comparison, 201907L);
#endif
#ifdef __cpp_lib_type_identity
		EXPECT_GE(__cpp_lib_type_identity, 201806L);
#endif
#ifdef __cpp_lib_unreachable
		EXPECT_GE(__cpp_lib_unreachable, 202202L);
#endif
#ifdef __cpp_lib_void_t
		EXPECT_GE(__cpp_lib_void_t, 201411L);
#endif
		SUCCEED();
	}

#undef EXPECT_FEATURE_AT_LEAST

}  // namespace
