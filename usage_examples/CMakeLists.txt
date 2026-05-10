add_executable(usage_examples
  "${CMAKE_CURRENT_SOURCE_DIR}/coroutines/coroutine_examples.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/RAII_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Factory_Method_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Abstract_Factory_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Builder_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Singleton_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Object_Pool_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Prototype_test.cpp"
)

set_property(TARGET usage_examples PROPERTY CXX_STANDARD 23)
target_compile_options(usage_examples PRIVATE /std:c++latest)

find_package(GTest REQUIRED)
target_link_libraries(usage_examples PRIVATE GTest::gtest GTest::gtest_main)
include(GoogleTest)