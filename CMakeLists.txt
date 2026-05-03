# CMakeList.txt : CMake project for VibeStdUnitTests, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.14)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("VibeStdUnitTests")

# Add source to this project's executable.
add_executable (VibeStdUnitTests "ranges_test.cpp")

set_property(TARGET VibeStdUnitTests PROPERTY CXX_STANDARD 23)

find_package(GTest REQUIRED)
target_link_libraries(VibeStdUnitTests PRIVATE GTest::gtest GTest::gtest_main)
include(GoogleTest)
