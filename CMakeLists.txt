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
add_executable (VibeStdUnitTests "VibeStdUnitTests_test.cpp")

set_property(TARGET VibeStdUnitTests PROPERTY CXX_STANDARD 20)
target_link_libraries(VibeStdUnitTests PRIVATE GTest::gtest_main)
include(GoogleTest)

# --- GoogleTest ---
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# For MSVC: keep runtime library consistent with the parent project
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

gtest_discover_tests(VibeStdUnitTests)
enable_testing()