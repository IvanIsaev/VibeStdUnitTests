add_executable(usage_examples
  "${CMAKE_CURRENT_SOURCE_DIR}/coroutines/coroutine_examples.cpp"
)

set_property(TARGET usage_examples PROPERTY CXX_STANDARD 23)
target_compile_options(usage_examples PRIVATE /std:c++latest)

find_package(GTest REQUIRED)
target_link_libraries(usage_examples PRIVATE GTest::gtest GTest::gtest_main)
include(GoogleTest)