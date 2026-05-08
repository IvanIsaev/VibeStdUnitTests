add_executable(std_vibe_unit_tests
  "${CMAKE_CURRENT_SOURCE_DIR}/cstdarg_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/csignal_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/csetjmp_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/ranges_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/algorithms_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/cstdlib_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/execution_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/cfloat_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/climits_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/compare_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/contracts_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/coroutine_test.cpp"
)

set_property(TARGET std_vibe_unit_tests PROPERTY CXX_STANDARD 23)
target_compile_options(std_vibe_unit_tests PRIVATE /std:c++latest)

find_package(GTest REQUIRED)
target_link_libraries(std_vibe_unit_tests PRIVATE GTest::gtest GTest::gtest_main)
include(GoogleTest)
