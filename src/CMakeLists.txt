add_executable(demo demo.cpp "../DataPoint.hpp")

target_link_libraries(
  demo
  PRIVATE
    project_options
    project_warnings
    fmt::fmt
    range-v3::range-v3)
