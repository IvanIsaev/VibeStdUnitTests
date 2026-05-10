add_executable(usage_examples
  "${CMAKE_CURRENT_SOURCE_DIR}/coroutines/coroutine_examples.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/RAII_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Factory_Method_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Abstract_Factory_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Builder_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Singleton_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Object_Pool_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Prototype_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Strategy_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Observer_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Visitor_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Command_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/State_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Template_Method_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Chain_of_Responsibility_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Iterator_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Mediator_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/behavioral/Memento_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/structural/PIMPL_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/structural/CRTP_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/structural/Adapter_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/structural/Decorator_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/structural/Facade_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/structural/Proxy_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/patterns/structural/Bridge_test.cpp"
)

set_property(TARGET usage_examples PROPERTY CXX_STANDARD 23)
target_compile_options(usage_examples PRIVATE /std:c++latest)

find_package(GTest REQUIRED)
target_link_libraries(usage_examples PRIVATE GTest::gtest GTest::gtest_main)
include(GoogleTest)