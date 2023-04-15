add_library(lib
  ast.h
  parser.h parser.cpp
  text_format.h text_format.cpp
  )

target_link_libraries(lib
  common
  absl::str_format
  absl::log absl::log_initialize absl::check
  )
target_include_directories(lib INTERFACE .)
