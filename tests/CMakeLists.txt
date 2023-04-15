project(tests)

add_executable(tests
  parser_tests.cpp
  text_format_tests.cpp
  )

target_link_libraries(tests
  common
  lib
  GTest::gtest_main
  GTest::gmock)
