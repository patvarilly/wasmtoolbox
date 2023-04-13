add_library(lib
  parser.h parser.cpp
  )

target_link_libraries(lib
  common
  absl::str_format
  absl::log absl::log_initialize absl::check
  )
target_include_directories(lib INTERFACE .)
