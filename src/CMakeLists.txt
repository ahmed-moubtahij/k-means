add_executable(demo demo.cpp)

target_link_libraries(
  demo
  PRIVATE
    kmn
    project_options
    project_warnings
    fmt::fmt
    range-v3::range-v3)
